/*
    SysmonForLinux

    Copyright (c) Microsoft Corporation

    All rights reserved.

    MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//====================================================================
//
// outputxml.cpp
//
// Event output formatting for JSON log file (/var/log/sysmon.json)
// Replaces previous XML syslog output.
//
//====================================================================

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/mman.h>
#include <limits.h>
#include <assert.h>

#include "linuxTypes.h"
#include "sysmonevents.h"
#include "linuxHelpers.h"
#include "sysmon_defs.h"

extern uint64_t *eventIdAddr;

//--------------------------------------------------------------------
//
// EscapeJsonString
//
// Escape a string for safe inclusion in a JSON string value.
//--------------------------------------------------------------------
static void EscapeJsonString(const char *src, char *dst, size_t dstSize)
{
    size_t di = 0;
    for (size_t si = 0; src[si] != '\0' && di + 1 < dstSize; si++) {
        char c = src[si];
        switch (c) {
            case '"':
                if (di + 2 < dstSize) { dst[di++] = '\\'; dst[di++] = '"'; }
                break;
            case '\\':
                if (di + 2 < dstSize) { dst[di++] = '\\'; dst[di++] = '\\'; }
                break;
            case '\n':
                if (di + 2 < dstSize) { dst[di++] = '\\'; dst[di++] = 'n'; }
                break;
            case '\r':
                if (di + 2 < dstSize) { dst[di++] = '\\'; dst[di++] = 'r'; }
                break;
            case '\t':
                if (di + 2 < dstSize) { dst[di++] = '\\'; dst[di++] = 't'; }
                break;
            default:
                if (di + 1 < dstSize) { dst[di++] = c; }
                break;
        }
    }
    dst[di] = '\0';
}

//--------------------------------------------------------------------
//
// FormatSyslogString
//
// Format an event into a JSON string and append to /var/log/sysmon.json.
//
//--------------------------------------------------------------------
extern "C"
VOID FormatSyslogString(
    PCHAR                           EventStr,
    size_t                          EventMax,
    CONST PSYSMON_EVENT_TYPE_FMT    EventType,
    CONST EVENT_DATA_DESCRIPTOR*    Fields,
    unsigned int                    FieldCount
    )
{
    if (EventStr != NULL) {
        *EventStr = 0x00;
    }

    if (EventStr == NULL || EventType == NULL || Fields == NULL) {
        fprintf(stderr, "FormatSyslogString invalid params\n");
        return;
    }

    uint64_t eventId = 0;
    unsigned int index = 0;
    const char *field = NULL;
    PCTSTR *fieldNames = NULL;
    char providerGuid[40];
    LARGE_INTEGER curTime;
    char systemTime[32];
    char hostname[HOST_NAME_MAX + 1];
    char tmpBuf[64];
    char escaped[512];

    if (eventIdAddr != NULL && eventIdAddr != MAP_FAILED) {
        eventId = (*eventIdAddr)++;
        msync(eventIdAddr, sizeof(eventId), MS_ASYNC);
    } else {
        eventId = 0;
    }

    assert(StringFromGUID2(SYSMON_PROVIDER, providerGuid, sizeof(providerGuid)) != 0);
    
    GetSystemTimeAsLargeInteger(&curTime);
    LargeIntegerToSystemTimeString(systemTime, 32, &curTime);

    if (gethostname(hostname, HOST_NAME_MAX + 1) < 0) {
        hostname[0] = 0x00;
    }

    // Build JSON string manually (single-line JSON object per event)
    int offset = 0;
    offset += snprintf(EventStr + offset, EventMax - offset, "{");
    offset += snprintf(EventStr + offset, EventMax - offset, "\"EventID\":%d,", EventType->EventDescriptor->Id);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"Version\":%d,", EventType->EventDescriptor->Version);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"Level\":%d,", EventType->EventDescriptor->Level);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"Task\":%d,", EventType->EventDescriptor->Task);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"Opcode\":%d,", EventType->EventDescriptor->Opcode);
    snprintf(tmpBuf, sizeof(tmpBuf), "0x%lx", EventType->EventDescriptor->Keyword);
    EscapeJsonString(tmpBuf, escaped, sizeof(escaped));
    offset += snprintf(EventStr + offset, EventMax - offset, "\"Keywords\":\"%s\",", escaped);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"TimeCreated\":\"%s\",", systemTime);
    snprintf(tmpBuf, sizeof(tmpBuf), "%lu", eventId);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"EventRecordID\":%s,", tmpBuf);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"ProcessID\":%d,", getpid());
    offset += snprintf(EventStr + offset, EventMax - offset, "\"ThreadID\":%d,", GetTid());
    EscapeJsonString("Linux-Sysmon/Operational", escaped, sizeof(escaped));
    offset += snprintf(EventStr + offset, EventMax - offset, "\"Channel\":\"%s\",", escaped);
    EscapeJsonString(hostname, escaped, sizeof(escaped));
    offset += snprintf(EventStr + offset, EventMax - offset, "\"Computer\":\"%s\",", escaped);
    offset += snprintf(EventStr + offset, EventMax - offset, "\"UserId\":%d,", geteuid());

    // EventData fields
    offset += snprintf(EventStr + offset, EventMax - offset, "\"EventData\":{");
    fieldNames = (PCTSTR *)EventType->FieldNames;
    for (index = 0; index < FieldCount; index++) {
        field = (const char *)Fields[index].Ptr;
        if (index > 0) {
            offset += snprintf(EventStr + offset, EventMax - offset, ",");
        }
        EscapeJsonString(fieldNames[index], escaped, sizeof(escaped));
        offset += snprintf(EventStr + offset, EventMax - offset, "\"%s\":", escaped);
        if (field != NULL) {
            EscapeJsonString(field, escaped, sizeof(escaped));
            offset += snprintf(EventStr + offset, EventMax - offset, "\"%s\"", escaped);
        } else {
            offset += snprintf(EventStr + offset, EventMax - offset, "\"\"");
        }
    }
    offset += snprintf(EventStr + offset, EventMax - offset, "}");

    offset += snprintf(EventStr + offset, EventMax - offset, "}");

    // Append JSON event to /var/log/sysmon.json
    FILE *logFile = fopen("/var/log/sysmon.json", "a");
    if (logFile != NULL) {
        fprintf(logFile, "%s\n", EventStr);
        fclose(logFile);
    } else {
        fprintf(stderr, "Failed to open /var/log/sysmon.json for writing\n");
    }
}

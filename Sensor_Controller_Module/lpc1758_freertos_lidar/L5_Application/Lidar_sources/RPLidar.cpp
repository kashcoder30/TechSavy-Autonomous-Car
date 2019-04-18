/*
 * RoboPeak RPLIDAR Driver for Arduino
 * RoboPeak.com
 *
 * Copyright (c) 2014, RoboPeak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stddef.h>
#include "lpc_sys.h"
#include "utilities.h"
#include "RPLidar.h"
#include "printf_lib.h"

#define distance_range 25.0

// ask the RPLIDAR for its health info
uint32_t RPLidar::getHealth(rplidar_response_device_health_t & healthinfo, uint32_t timeout)
{
   // u0_dbg_printf("step 1\n");
    uint32_t currentTs = sys_get_uptime_ms();
    uint32_t remainingtime;
    uint8_t *infobuf = (uint8_t *)&healthinfo;
    uint8_t recvPos = 0;

    rplidar_ans_header_t response_header;
    u_result  ans;
  //  u0_dbg_printf("step 2 %x\n",ans);
        if (IS_FAIL(ans = _sendCommand(RPLIDAR_CMD_GET_DEVICE_HEALTH, NULL, 0))) {
          //  u0_dbg_printf("step 3\n");
            return ans;
        }

        if (IS_FAIL(ans = _waitResponseHeader(&response_header, timeout))) {
           // u0_dbg_printf("step 4\n");
            return ans;
        }

       // u0_dbg_printf("response header type %x\n",response_header.type);
         //   u0_dbg_printf("response header size %x\n",response_header.size);
           // u0_dbg_printf("response header sync %x\n",response_header.syncByte1);
        // verify whether we got a correct header
        if (response_header.type != RPLIDAR_ANS_TYPE_DEVHEALTH) {
            return RESULT_INVALID_DATA;
        }

        if ((response_header.size) < sizeof(rplidar_response_device_health_t)) {
            return RESULT_INVALID_DATA;
        }

        while ((remainingtime=sys_get_uptime_ms() - currentTs) <= timeout) {
            char received_data;
            uart2.getChar(&received_data,portMAX_DELAY);
            int currentbyte = received_data;
            if (currentbyte < 0) continue;
         //   u0_dbg_printf("currentbyte in gethealth%d\n",currentbyte);
            infobuf[recvPos++] = currentbyte;

            if (recvPos == sizeof(rplidar_response_device_health_t)) {
                return RESULT_OK;
            }
        }
    //u0_dbg_printf("%d\n",RESULT_OPERATION_TIMEOUT);
    return RESULT_OPERATION_TIMEOUT;
}

// ask the RPLIDAR for its device info like the serial number
uint32_t RPLidar::getDeviceInfo(rplidar_response_device_info_t & info, uint32_t timeout )
{
    uint8_t  recvPos = 0;
    uint32_t currentTs = sys_get_uptime_ms();
    uint32_t remainingtime;
    uint8_t *infobuf = (uint8_t*)&info;
    rplidar_ans_header_t response_header;
    u_result  ans;

    {
        if (IS_FAIL(ans = _sendCommand(RPLIDAR_CMD_GET_DEVICE_INFO,NULL,0))) {
            return ans;
        }

        if (IS_FAIL(ans = _waitResponseHeader(&response_header, timeout))) {
            return ans;
        }

        // verify whether we got a correct header
        if (response_header.type != RPLIDAR_ANS_TYPE_DEVINFO) {
            return RESULT_INVALID_DATA;
        }

        if (response_header.size < sizeof(rplidar_response_device_info_t)) {
            return RESULT_INVALID_DATA;
        }

        while ((remainingtime=sys_get_uptime_ms() - currentTs) <= timeout) {

            char received_data;
            uart2.getChar(&received_data,portMAX_DELAY);
            int currentbyte = received_data;
            if (currentbyte<0) continue;
            infobuf[recvPos++] = currentbyte;

            if (recvPos == sizeof(rplidar_response_device_info_t)) {
                return RESULT_OK;
            }
        }
    }

    return RESULT_OPERATION_TIMEOUT;
}

// stop the measurement operation
uint32_t RPLidar::stop()
{
    uint32_t ans = _sendCommand(RPLIDAR_CMD_STOP,NULL,0);
    return ans;
}

uint32_t RPLidar::reset()
{
    uint32_t ans = _sendCommand(RPLIDAR_CMD_RESET,NULL,0);
    return ans;
}


// start the measurement operation
uint32_t RPLidar::startScan(bool force, uint32_t timeout)
{
    uint32_t ans;

    stop(); //force the previous operation to stop
    u0_dbg_printf("step1\n");
    ans = _sendCommand(force?RPLIDAR_CMD_FORCE_SCAN:RPLIDAR_CMD_SCAN, NULL, 0);
        if (IS_FAIL(ans)) return ans;
        u0_dbg_printf("step2\n");

        // waiting for confirmation
        rplidar_ans_header_t response_header;
        if (IS_FAIL(ans = _waitResponseHeader(&response_header, timeout))) {
            u0_dbg_printf("ans is %x\n",ans);
            u0_dbg_printf("step3\n");
            return ans;
        }

        // verify whether we got a correct header
        if (response_header.type != RPLIDAR_ANS_TYPE_MEASUREMENT) {
            u0_dbg_printf("step4\n");
            return RESULT_INVALID_DATA;
        }

        if (response_header.size < sizeof(rplidar_response_measurement_node_t)) {
            u0_dbg_printf("step5\n");
            return RESULT_INVALID_DATA;
        }
        u0_dbg_printf("step6\n");
    return RESULT_OK;
}



uint32_t RPLidar::_sendCommand(uint8_t cmd, const void * payload, size_t payloadsize)
{
    rplidar_cmd_packet_t pkt_header;
    rplidar_cmd_packet_t * header = &pkt_header;

    if (payloadsize && payload) {
        cmd |= RPLIDAR_CMDFLAG_HAS_PAYLOAD;
    }

    header->syncByte = RPLIDAR_CMD_SYNC_BYTE;
    header->cmd_flag = cmd;
    uart2.putChar(header->syncByte, portMAX_DELAY);
    uart2.putChar(header->cmd_flag, portMAX_DELAY);
    return RESULT_OK;
}

uint32_t RPLidar::_waitResponseHeader(rplidar_ans_header_t * header, uint32_t timeout)
{
    uint8_t  recvPos = 0;
    uint32_t currentTs = sys_get_uptime_ms();
    uint32_t remainingtime;
    uint8_t *headerbuf = (uint8_t*)header;
    while ((remainingtime=sys_get_uptime_ms() - currentTs) <= timeout) {

        char received_data;
       // u0_dbg_printf("check current %x\n",uart2.getChar(&received_data,portMAX_DELAY));
        uart2.getChar(&received_data,portMAX_DELAY);
        int currentbyte = received_data;

        if (currentbyte<0) continue;

        switch (recvPos) {
        case 0:
            if (currentbyte != RPLIDAR_ANS_SYNC_BYTE1) {
                continue;
            }
            break;
        case 1:
            if (currentbyte != RPLIDAR_ANS_SYNC_BYTE2) {
                recvPos = 0;
                continue;
            }
            break;
        }
        headerbuf[recvPos++] = currentbyte;
        //u0_dbg_printf("inside waitresponse buff is %x\n",headerbuf[recvPos++]);
        if (recvPos == sizeof(rplidar_ans_header_t)) {
            return RESULT_OK;
        }


    }

    return RESULT_OPERATION_TIMEOUT;
}

uint32_t RPLidar::waitPoint(rplidar_response_measurement_node_t *node, uint32_t timeout, RPLidarMeasurement *measurement)
{
    uint32_t currentTs = sys_get_uptime_ms();
    uint32_t remainingtime;
    uint8_t *nodebuf = (uint8_t*)node;
    uint8_t recvPos = 0;
   while ((remainingtime=sys_get_uptime_ms() - currentTs) <= timeout) {
       char recieved_char;
       int currentbyte = 0;
       if(uart2.getChar(&recieved_char,portMAX_DELAY)){
           currentbyte = recieved_char;
       }
        if (currentbyte<0) continue;

        switch (recvPos) {
            case 0: // expect the sync bit and its reverse in this byte          {
                {
                    uint8_t tmp = (currentbyte>>1);
                    if ( (tmp ^ currentbyte) & 0x1 ) {
                        // pass
                    } else {
                        continue;
                    }

                }
                break;
            case 1: // expect the highest bit to be 1
                {
                    if (currentbyte & RPLIDAR_RESP_MEASUREMENT_CHECKBIT) {
                        // pass
                    } else {
                        recvPos = 0;
                        continue;
                    }
                }
                break;
          }
          nodebuf[recvPos++] = currentbyte;

          if (recvPos == sizeof(rplidar_response_measurement_node_t)) {
              measurement->distance = node->distance_q2/(float)4.0;
              measurement->angle = (node->angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)/(float)64.0;
              measurement->quality = (node->sync_quality>>RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
              measurement->startBit = (node->sync_quality & RPLIDAR_RESP_MEASUREMENT_SYNCBIT);
              return RESULT_OK;
          }
   }
   return RESULT_OPERATION_TIMEOUT;
}

int RPLidar::divideDistance(float distance)
{
    int track;
    if(distance > 27 && distance<= distance_range*2 ){
            track = 1;
        }
        else if(distance > distance_range*2 && distance <= distance_range*3){
            track = 2;
        }
        else if(distance > distance_range*3 && distance <= distance_range*4){
            track = 3;
        }
        else if(distance > distance_range*4 && distance <= distance_range*5){
            track = 4;
        }
        else if(distance > distance_range*5 && distance <= distance_range*6){
            track = 5;
        }
        else if(distance > distance_range*6 && distance <= distance_range*7){
            track = 6;
        }
        else if(distance > distance_range*7 && distance <= distance_range*8){
              track = 7;
        }
        else{
            track = 0;
        }
        return track;
}
uint32_t RPLidar::divideAngle(RPLidarMeasurement *angle_value,int length, store_SECTION_value *identify_section)
{
    int check_track = 0;
    int store_angle = 0;

    for(int i=0; i<8; i++)
    {
    store_angle = angle_value[i].angle;
    if(store_angle > 360 || store_angle < 0 )
    {
        return RESULT_INVALID_DATA;
    }
    else
    {
        switch(store_angle)
        {
        case 0 ... SECTION0:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section0 = check_track;
            u0_dbg_printf("in section 0 %d %d\n",check_track,identify_section->section0);
            break;
        }
        case SECTION0+1 ... SECTION1:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section1 = check_track;
           // u0_dbg_printf("in section 1 %d\n",check_track);

            break;
        }
        case SECTION1+1 ... SECTION2:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section2 = check_track;
            break;
        }
        case SECTION2+1 ... SECTION3:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section3 = check_track;
            break;
        }
        case SECTION3+1 ... SECTION4:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section4 = check_track;
            break;
        }
        case SECTION4+1 ... SECTION5:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section5 = check_track;
            break;
        }
        case SECTION5+1 ... SECTION6:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section6 = check_track;
            break;
        }
        case SECTION6+1 ... SECTION7:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section7 = check_track;
            break;
        }
        case SECTION7+1 ... SECTION8:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section8 = check_track;
            break;
        }
        case SECTION8+1 ... SECTION9:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section9 = check_track;
            break;
        }
        case SECTION9+1 ... SECTION10:
        {
            check_track = divideDistance(angle_value->distance);
            identify_section->section10 = check_track;
            break;
        }
        }
        return RESULT_OK;
    }
    }
}
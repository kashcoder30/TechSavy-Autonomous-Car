/*
 * LidarTask_create.cpp
 *
 *  Created on: Apr 7, 2019
 *      Author: halak
 */

#include "LidarTask_create.h"
#include "scheduler_task.hpp"
#include "LidarTask.h"
#include "L5_Application/ultrasonic_sensor.h"

static Lidar lidar_obj;
LidarTask::LidarTask(uint8_t priority):scheduler_task("LidarTask", 2000, priority)
{}

bool LidarTask::init()
{
    lidar_obj.Lidar_init();
    sensor_cntlr_init();
    return true;
}

bool LidarTask::run(void *p)
{

    lidar_obj.Lidar_get_data();
    lidar_obj.Lidar_parse_data();
    lidar_obj.Lidar_send_data_CAN();
    return true;
}


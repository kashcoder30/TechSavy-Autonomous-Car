/*
 * lcd.cpp
 *
 *  Created on: Apr 2, 2019
 *      Author: Aakash
 */

#include "lcd.h"
#include <stdio.h>
#include "c_uart3_wrapper.h"
#include <string.h>

lcd_data_packet_t lcd_screen_id = {0};
static lcd_screen_t lcd_screen = home;

#define SENSOR_RIGHT_OBJ        0x00
#define SENSOR_FRONT_OBJ        0x01
#define SENSOR_LEFT_OBJ         0x02
#define SENSOR_BACK_OBJ         0x03

#define GPS_LATITUDE_OBJ        0
#define GPS_LONGITUDE_OBJ       1
#define GPS_BEARING_OBJ         2
#define GPS_DISTANCE_OBJ        3

extern char led_digit_front_obs;
extern char led_digit_left_obs;
extern char led_digit_right_obs;
extern char led_digit_back_obs;

extern float gps_latitude;
extern float gps_longitude;
extern float gps_bearing;
extern float gps_distance;

void lcd_init(void)
{
    uart3_init(LCD_BAUD_RATE, LCD_RX_Q_SIZE, LCD_TX_Q_SIZE);
}

void lcd_screen_query(void)
{
    char comm[] = {0, 0x0a, 0, 0x0a};
    for (int i = 0; i < (int)sizeof(comm); i++)
    {
        uart3_put_char(comm[i], 0);
    }
}

void lcd_put_comm(char event_id, char obj_id, char obj_num, char data_msb, char data_lsb)
{
    char checksum = event_id ^ obj_id ^ obj_num ^ data_msb ^ data_lsb;
    uart3_put_char(event_id, 0);
    uart3_put_char(obj_id, 0);
    uart3_put_char(obj_num, 0);
    uart3_put_char(data_msb, 0);
    uart3_put_char(data_lsb, 0);
    uart3_put_char(checksum, 0);
   // uart3_get_char(&lcd_screen_id.ack, 0);
}

void lcd_put_string(char obj_num, char *data, int data_size)
{
    char checksum = 0x02 ^ obj_num ^ data_size;
    uart3_put_char(0x02, 0);
    uart3_put_char(obj_num, 0);
    uart3_put_char(data_size, 0);

    for (int i = 0; i < data_size; i++)
    {
        uart3_put_char(data[i], 0);
        checksum ^= data[i];
    }
    uart3_put_char(checksum, 0);
   // uart3_get_char(&lcd_screen_id.ack, 0);
  }

void lcd_receive(void)
{
    uart3_get_char(&lcd_screen_id.event_id, 0);
    uart3_get_char(&lcd_screen_id.object_id, 0);
    uart3_get_char(&lcd_screen_id.object_num, 0);
    uart3_get_char(&lcd_screen_id.data_msb, 0);
    uart3_get_char(&lcd_screen_id.data_lsb, 0);
    uart3_get_char(&lcd_screen_id.checksum, 0);
    lcd_get_screen_id(lcd_screen_id.data_lsb);
//    printf("%x %x %x %x %x\n", lcd_screen_id.event_id
//            , lcd_screen_id.object_id
//            , lcd_screen_id.object_num
//            , lcd_screen_id.data_msb
//            , lcd_screen_id.data_lsb);
}

void lcd_get_screen_id(char screen_id)
{
    u0_dbg_printf("ScreenId: %d\n",screen_id);
    switch(screen_id)
    {
        case 0:
            lcd_print_menu_screen();
            lcd_screen = menu;
            break;
        case 1:
            lcd_print_bridge_screen();
            lcd_screen = bridge;
            break;
        case 2:
            lcd_print_gps_screen();
            lcd_screen = gps;
            break;
        case 3:
            lcd_print_sensor_screen();
            lcd_screen = sensor;
            break;
        case 4:
            lcd_print_motor_screen();
            lcd_screen = motor;
            break;
        case 5:
            lcd_print_home_screen();
            lcd_screen = home;
            break;
        case 6:
            lcd_print_mia_screen();
            lcd_screen = mia;
            break;
        default:
            break;
    }
}

void lcd_print()
{
    switch (lcd_screen)
    {
        case menu:
            lcd_print_menu_screen();
            break;
        case bridge:
            lcd_print_bridge_screen();
            break;
        case gps:
            lcd_print_gps_screen();
            break;
        case sensor:
            lcd_print_sensor_screen();
            break;
        case motor:
            lcd_print_motor_screen();
            break;
        case home:
            lcd_print_home_screen();
            break;
        case mia:
            lcd_print_mia_screen();
            break;
        default:
            break;
    }
}

void lcd_print_menu_screen()
{

}

void lcd_print_bridge_screen()
{

}

void lcd_print_gps_screen()
{
    static char string[30];
//    double latitude = 29.783493;
    sprintf(string, "%f", gps_latitude);
    lcd_put_string(GPS_LATITUDE_OBJ, string, (int)strlen(string));

    sprintf(string, "%f", gps_longitude);
    lcd_put_string(GPS_LONGITUDE_OBJ, string, (int)strlen(string));

    sprintf(string, "%f", gps_bearing);
    lcd_put_string(GPS_BEARING_OBJ, string, (int)strlen(string));

    sprintf(string, "%f", gps_distance);
    lcd_put_string(GPS_DISTANCE_OBJ, string, (int)strlen(string));

    lcd_put_comm(WRITE_OBJ, ANGULAR_METER, 0x00, 0x00, (char)gps_bearing);
}

void lcd_print_motor_screen()
{

}

void lcd_print_sensor_screen()
{
    lcd_put_comm(WRITE_OBJ, LED_DIGITS, SENSOR_RIGHT_OBJ, 0x00, led_digit_right_obs);
    lcd_put_comm(WRITE_OBJ, LED_DIGITS, SENSOR_FRONT_OBJ, 0x00, led_digit_front_obs);
    lcd_put_comm(WRITE_OBJ, LED_DIGITS, SENSOR_LEFT_OBJ, 0x00, led_digit_left_obs);
    lcd_put_comm(WRITE_OBJ, LED_DIGITS, SENSOR_BACK_OBJ, 0x00, led_digit_back_obs);
}

void lcd_print_home_screen()
{

}

extern uint8_t master_status, sensor_status, gps_status, bridge_status;

void lcd_print_mia_screen()
{
    //static char led_data = 0x00;
    lcd_put_comm(WRITE_OBJ, USER_LED, 0x05, 0x00, master_status);
    lcd_put_comm(WRITE_OBJ, USER_LED, 0x06, 0x00, sensor_status);
    lcd_put_comm(WRITE_OBJ, USER_LED, 0x07, 0x00, sensor_status);
    lcd_put_comm(WRITE_OBJ, USER_LED, 0x08, 0x00, bridge_status);
    //lcd_put_comm(WRITE_OBJ, USER_LED, 0x09, 0x00, led_data);
    //led_data = !led_data;
}



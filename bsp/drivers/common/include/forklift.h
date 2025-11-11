#pragma once
#include <linux/ioctl.h>

/* 공통 IOCTL 매직 */
#define FK_MAGIC   'F'

/* motor */
#define IOCTL_MOTOR_SET_SPEED _IOW(FK_MAGIC, 1, int)   /* 0~100 */
#define IOCTL_MOTOR_SET_DIR   _IOW(FK_MAGIC, 2, int)   /* 0:stop 1:fwd 2:rev */
#define IOCTL_MOTOR_GET_STATE _IOR(FK_MAGIC, 3, int[2])/* {dir,speed} */

/* encoder (가상) */
struct fk_enc_stat { unsigned int ticks; unsigned int speed; }; /* 임의단위 */
#define IOCTL_ENC_SET_SPEED   _IOW(FK_MAGIC, 10, unsigned int)
#define IOCTL_ENC_GET_STAT    _IOR(FK_MAGIC, 11, struct fk_enc_stat)

/* estop */
#define IOCTL_ESTOP_TRIP      _IO(FK_MAGIC, 20)        /* 비상정지 ON */
#define IOCTL_ESTOP_CLEAR     _IO(FK_MAGIC, 21)        /* 해제 */
#define IOCTL_ESTOP_GET       _IOR(FK_MAGIC, 22, int)  /* 0/1 */

/* ledkey (8비트) */
#define IOCTL_LED_SET         _IOW(FK_MAGIC, 30, unsigned char) /* bit=LED */
#define IOCTL_LED_GET         _IOR(FK_MAGIC, 31, unsigned char)
#define IOCTL_KEY_GET         _IOR(FK_MAGIC, 32, unsigned char)

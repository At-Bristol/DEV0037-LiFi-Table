/*! 
 * \file      purelifi_log.h
 * \brief     linux
 * \details   Debug log message
 * \author    Angelos Spanos
 * \version   1.0
 * \date      21-Mar-2016
 * \copyright Pure LiFi
 */

#ifndef _PURELIFI_LOG
    #define _PURELIFI_LOG

    #ifdef PL_DEBUG
        #define pl_dev_info(dev, fmt, arg...) dev_info(dev, fmt, ##arg)
        #define pl_dev_warn(dev, fmt, arg...) dev_warn(dev, fmt, ##arg)
        #define  pl_dev_err(dev, fmt, arg...) dev_err (dev, fmt, ##arg)
    #else
        #define pl_dev_info(dev, fmt, arg...) do { (void)(dev); } while (0)
        #define pl_dev_warn(dev, fmt, arg...) do { (void)(dev); } while (0)
        #define  pl_dev_err(dev, fmt, arg...) do { (void)(dev); } while (0)
    #endif
#endif

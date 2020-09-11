-- MySQL dump 10.15  Distrib 10.0.30-MariaDB, for Linux (x86_64)
--
-- Host: localhost    Database: mtreport_db
-- ------------------------------------------------------
-- Server version	10.0.30-MariaDB

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `mtreport_db`
--

/*!40000 DROP DATABASE IF EXISTS `mtreport_db`*/;

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `mtreport_db` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `mtreport_db`;

--
-- Table structure for table `flogin_history`
--

DROP TABLE IF EXISTS `flogin_history`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `flogin_history` (
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(11) unsigned NOT NULL,
  `login_time` int(12) unsigned DEFAULT '0',
  `login_remote_address` char(16) DEFAULT '0.0.0.0',
  `receive_server` char(16) DEFAULT '0.0.0.0',
  `referer` varchar(1024) DEFAULT NULL,
  `user_agent` varchar(1024) DEFAULT NULL,
  `method` int(10) DEFAULT '0',
  `valid_time` int(11) unsigned DEFAULT NULL,
  PRIMARY KEY (`xrk_id`),
  KEY `user_id` (`user_id`)
) ENGINE=MyISAM AUTO_INCREMENT=320 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `flogin_history`
--

LOCK TABLES `flogin_history` WRITE;
/*!40000 ALTER TABLE `flogin_history` DISABLE KEYS */;
INSERT INTO `flogin_history` VALUES (319,1,1599725273,'192.168.128.1','192.168.128.132','http://192.168.128.132/cgi-bin/slog_flogin?action=show_main&login_show=var_css','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:79.0) Gecko/20100101 Firefox/79.0',0,1600330073);
/*!40000 ALTER TABLE `flogin_history` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `flogin_user`
--

DROP TABLE IF EXISTS `flogin_user`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `flogin_user` (
  `user_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `user_name` varchar(64) NOT NULL,
  `user_pass_md5` varchar(128) NOT NULL,
  `login_type` int(11) NOT NULL DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `rmark` varchar(128) DEFAULT NULL,
  `dwReserved1` int(11) unsigned DEFAULT '0',
  `dwReserved2` int(11) unsigned DEFAULT '0',
  `strReserved1` varchar(64) DEFAULT NULL,
  `strReserved2` varchar(64) DEFAULT NULL,
  `user_flag_1` int(12) unsigned DEFAULT '0',
  `last_login_time` int(12) unsigned DEFAULT '0',
  `register_time` int(12) unsigned DEFAULT '0',
  `last_login_address` char(16) DEFAULT '0.0.0.0',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `xrk_status` tinyint(4) DEFAULT '0',
  `email` varchar(64) DEFAULT NULL,
  `login_index` int(11) DEFAULT '0',
  `login_md5` varchar(32) DEFAULT '',
  `last_login_server` char(16) DEFAULT '0.0.0.0',
  `bind_xrkmonitor_uid` int(11) unsigned DEFAULT '0',
  PRIMARY KEY (`user_id`),
  KEY `idx_name` (`user_name`)
) ENGINE=InnoDB AUTO_INCREMENT=357 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `flogin_user`
--

LOCK TABLES `flogin_user` WRITE;
/*!40000 ALTER TABLE `flogin_user` DISABLE KEYS */;
INSERT INTO `flogin_user` VALUES (1,'sadmin','c5edac1b8c1d58bad90a246d8f08f53b',1,'2020-09-10 08:07:53','supperuser',1552345835,152,NULL,'0c96aa43ce31757758c40be976c20d34',1,1599725273,0,'192.168.128.1',1,1,0,'4033@qq.com',0,'c5edac1b8c1d58bad90a246d8f08f53b','192.168.128.132',0);
/*!40000 ALTER TABLE `flogin_user` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger flogin_user_up after update on flogin_user for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('flogin_user', old.user_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_app_info`
--

DROP TABLE IF EXISTS `mt_app_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_app_info` (
  `app_name` varchar(32) NOT NULL,
  `app_desc` varchar(128) NOT NULL,
  `app_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `create_time` int(12) unsigned DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_add` varchar(64) NOT NULL,
  `user_mod` varchar(64) NOT NULL,
  `xrk_status` tinyint(4) DEFAULT '0',
  `app_type` int(10) unsigned DEFAULT '2',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  PRIMARY KEY (`app_id`)
) ENGINE=InnoDB AUTO_INCREMENT=120 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_app_info`
--

LOCK TABLES `mt_app_info` WRITE;
/*!40000 ALTER TABLE `mt_app_info` DISABLE KEYS */;
INSERT INTO `mt_app_info` VALUES ('ç›‘æŽ§ç³»ç»Ÿ','ç›‘æŽ§ç³»ç»Ÿ',30,0,'2019-01-25 23:42:34','sadmin','sadmin',0,3,1,1),('ç›‘æŽ§æ’ä»¶','å…¨éƒ¨æ’ä»¶å½’å±žåˆ°è¯¥åº”ç”¨ä¸‹ï¼Œæ¯ä¸ªæ’ä»¶ä»¥æ’ä»¶ååœ¨è¯¥åº”ç”¨ä¸‹åˆ›å»ºæ¨¡å—',119,1584529976,'2020-03-18 11:12:56','sadmin','sadmin',0,2,1,1);
/*!40000 ALTER TABLE `mt_app_info` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_app_info_ins after insert on mt_app_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_app_info', new.app_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_app_info_up after update on mt_app_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_app_info', old.app_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_attr`
--

DROP TABLE IF EXISTS `mt_attr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_attr` (
  `xrk_id` int(11) NOT NULL AUTO_INCREMENT,
  `attr_type` int(8) DEFAULT '0' COMMENT 'å±žæ€§ç±»åž‹',
  `data_type` int(6) DEFAULT '0' COMMENT 'æ•°æ®ç±»åž‹',
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `attr_name` varchar(64) NOT NULL COMMENT 'å±žæ€§åç§°',
  `attr_desc` varchar(256) DEFAULT NULL COMMENT 'å±žæ€§æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_add_id` int(11) unsigned DEFAULT '0',
  `user_mod_id` int(11) unsigned DEFAULT '0',
  `excep_attr_mask` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`xrk_id`)
) ENGINE=InnoDB AUTO_INCREMENT=358 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_attr`
--

LOCK TABLES `mt_attr` WRITE;
/*!40000 ALTER TABLE `mt_attr` DISABLE KEYS */;
INSERT INTO `mt_attr` VALUES (62,51,1,'sadmin','æ¨¡å—æ—¥å¿—è®°å½•å‘é€é‡','æ¨¡å—æ—¥å¿—è®°å½•å‘é€é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(64,51,1,'sadmin','æ”¶åˆ°å¤±è´¥çš„æ—¥å¿—å“åº”åŒ…é‡','æ”¶åˆ°å¤±è´¥çš„æ—¥å¿—å“åº”åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(65,51,1,'sadmin','æ”¶åˆ°æ—¥å¿—å“åº”åŒ…é‡','æ”¶åˆ°æ—¥å¿—å“åº”åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(67,55,1,'sadmin','å†™å…¥æ—¥å¿—è®°å½•é‡','å†™å…¥æ—¥å¿—è®°å½•é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(70,51,1,'sadmin','æ—¥å¿—å‘é€åŒ…é‡','æ—¥å¿—å‘é€åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(73,53,1,'sadmin','pb æ ¼å¼æ—¥å¿—åŒ…é‡','pb æ ¼å¼æ—¥å¿—åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(74,53,3,'sadmin','æ”¶åˆ°éžæ³•æ—¥å¿—åŒ…é‡','æ”¶åˆ°éžæ³•æ—¥å¿—åŒ…é‡',0,1,1,1,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(75,53,1,'sadmin','æ”¶åˆ°çš„æ—¥å¿—è®°å½•é‡','æ”¶åˆ°çš„æ—¥å¿—è®°å½•é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(84,55,1,'sadmin','è¾“å‡ºæ—¥å¿—æ–‡ä»¶å ç”¨ç£ç›˜ç©ºé—´åˆ°æ–‡ä»¶','è¾“å‡ºæ—¥å¿—æ–‡ä»¶å ç”¨ç£ç›˜ç©ºé—´åˆ°æ–‡ä»¶',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(93,51,1,'sadmin','slog_client å¿ƒè·³ä¸ŠæŠ¥é‡','æ—¥å¿—å®¢æˆ·ç«¯å¿ƒè·³ä¸ŠæŠ¥',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(94,53,1,'sadmin','æ”¶åˆ°Logå¿ƒè·³ä¸ŠæŠ¥é‡','æ¥è‡ª slog_client çš„å¿ƒè·³',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(100,67,1,'sadmin','æ•°æ®å­˜å…¥vmemé‡','æ•°æ®å­˜å…¥vmemé‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(101,67,3,'sadmin','æ•°æ®å­˜å…¥vmemå¤±è´¥é‡','æ•°æ®å­˜å…¥vmemå¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(102,67,1,'sadmin','ä»Žvmem å–å‡ºæ•°æ®æˆåŠŸé‡','ä»Žvmem å–å‡ºæ•°æ®æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(103,67,3,'sadmin','ä»Žvmem å–å‡ºæ•°æ®å¼‚å¸¸','ä»Žvmem å–å‡ºæ•°æ®å¼‚å¸¸',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(104,67,3,'sadmin','vmem é‡Šæ”¾å¼‚å¸¸','vmem é‡Šæ”¾å¼‚å¸¸',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(105,67,1,'sadmin','vmem å»¶è¿Ÿé‡Šæ”¾é‡','vmem å»¶è¿Ÿé‡Šæ”¾é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(106,67,1,'sadmin','vmem å»¶è¿Ÿé‡Šæ”¾æˆåŠŸ','vmem å»¶è¿Ÿé‡Šæ”¾',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(108,69,3,'sadmin','cgi å¯åŠ¨å¤±è´¥','cgi æ‰§è¡Œå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(110,69,1,'sadmin','login cookie check å¤±è´¥','login cookie check å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(111,70,1,'sadmin','login cookie check æˆåŠŸé‡','login cookie check æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(112,70,1,'sadmin','login å¼¹å‡ºç™»å½•æ¡†','login å¼¹å‡ºç™»å½•æ¡†',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(113,70,1,'sadmin','login é€šè¿‡è¾“å…¥ç”¨æˆ·åå¯†ç ç™»å½•æˆåŠŸ','login é€šè¿‡è¾“å…¥ç”¨æˆ·åå¯†ç ç™»å½•æˆåŠŸ',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(116,71,3,'sadmin','mysql è¿žæŽ¥å¤±è´¥','mysql è¿žæŽ¥å¤±è´¥',0,1,1,1,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(117,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶0-10ms','mysql è¿žæŽ¥è€—æ—¶0-10ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(118,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶10-20ms','mysql è¿žæŽ¥è€—æ—¶10-20ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(119,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶20-50ms','mysql è¿žæŽ¥è€—æ—¶20-50ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(120,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶50-100ms','mysql è¿žæŽ¥è€—æ—¶50-100ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(121,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶å¤§äºŽ1000ms','mysql è¿žæŽ¥è€—æ—¶å¤§äºŽ1000ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(122,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´ 0-20 ms','mysql æ‰§è¡Œæ—¶é—´ 0-20 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(123,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´ 20-50 ms','mysql æ‰§è¡Œæ—¶é—´ 20-50 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(124,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´ 50-100 ms','mysql æ‰§è¡Œæ—¶é—´ 50-100 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(125,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´ 100-200 ms','mysql æ‰§è¡Œæ—¶é—´ 100-200 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(126,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´ 200-500 ms','mysql æ‰§è¡Œæ—¶é—´ 200-500 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(127,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´ 500-1000 ms','mysql æ‰§è¡Œæ—¶é—´ 500-1000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(128,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´ 1000-2000 ms','mysql æ‰§è¡Œæ—¶é—´ 1000-2000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(129,71,1,'sadmin','mysql æ‰§è¡Œæ—¶é—´å¤§äºŽ 2000','mysql æ‰§è¡Œæ—¶é—´å¤§äºŽ 2000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(130,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶åœ¨ 100-200 ms','mysql è¿žæŽ¥è€—æ—¶åœ¨ 100-200 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(131,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶åœ¨ 200-500 ms','mysql è¿žæŽ¥è€—æ—¶åœ¨ 200-500 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(132,71,1,'sadmin','mysql è¿žæŽ¥è€—æ—¶åœ¨ 500-1000 ms','mysql è¿žæŽ¥è€—æ—¶åœ¨ 500-1000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(133,71,1,'sadmin','mysql get result å®Œæˆæ—¶é—´ 0-20ms','mysql get result å®Œæˆæ—¶é—´ 0-20ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(134,71,1,'sadmin','mysql get result å®Œæˆæ—¶é—´ 20-50ms','mysql get result å®Œæˆæ—¶é—´ 20-50ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(135,71,1,'sadmin','mysql get result å®Œæˆæ—¶é—´ 50-100ms','mysql get result å®Œæˆæ—¶é—´ 50-100ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(136,71,1,'sadmin','mysql get result å®Œæˆæ—¶é—´ 100-200ms','mysql get result å®Œæˆæ—¶é—´ 100-200ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(137,71,1,'sadmin','mysql get result å®Œæˆæ—¶é—´ 200-500ms','mysql get result å®Œæˆæ—¶é—´ 200-500ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(138,71,1,'sadmin','mysql get result å®Œæˆæ—¶é—´ 500-1000ms','mysql get result å®Œæˆæ—¶é—´ 500-1000ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(139,71,1,'sadmin','mysql get result å®Œæˆæ—¶é—´å¤§äºŽ1000ms','mysql get result å®Œæˆæ—¶é—´å¤§äºŽ1000ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(142,69,1,'sadmin','cgi è¯·æ±‚å¤±è´¥é‡','cgi è¯·æ±‚å¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(143,70,1,'sadmin','cgi è¯·æ±‚æˆåŠŸé‡','cgi è¯·æ±‚æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(144,72,1,'sadmin','cgi å“åº”è€—æ—¶åœ¨ 0-100ms ','cgi å“åº”è€—æ—¶åœ¨ 0-100ms ',0,1,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(145,72,1,'sadmin','cgi å“åº”è€—æ—¶åœ¨ 100-300 ms','cgi å“åº”è€—æ—¶åœ¨ 100-300 ms',0,15,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(146,72,1,'sadmin','cgi å“åº”è€—æ—¶åœ¨ 300-500 ms','cgi å“åº”è€—æ—¶åœ¨ 300-500 ms',0,15,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(147,72,1,'sadmin','cgi å“åº”è€—æ—¶å¤§äºŽç­‰äºŽ 500 ms','cgi å“åº”è€—æ—¶å¤§äºŽç­‰äºŽ 500 ms',0,15,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(203,80,1,'sadmin','memcache get æˆåŠŸé‡','memcache get æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(204,80,1,'sadmin','memcache get æ€»é‡','memcache get æ€»é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(205,80,1,'sadmin','memcache get å¤±è´¥é‡','memcache get å¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(206,80,1,'sadmin','memcache set å¤±è´¥é‡','memcache set å¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(207,80,1,'sadmin','memcache set æˆåŠŸé‡','memcache set æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(208,80,1,'sadmin','memcache set æ€»é‡','memcache set æ€»é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(220,53,1,'sadmin','client logä¸ŠæŠ¥æ—¶é—´è§¦å‘æ ¡å‡†','client ä¸Ž server çš„æ—¶é—´ç›¸å·®è¶…è¿‡2åˆ†é’Ÿ',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(223,82,1,'sadmin','é‚®ä»¶è¿‡æœŸæœªå‘é€é‡','é‚®ä»¶è¿‡æœŸæœªå‘é€é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(224,82,1,'sadmin','é‚®ä»¶å‘é€é‡','æ€»é‚®ä»¶å‘é€é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(225,82,1,'sadmin','é‚®ä»¶å‘é€å¤±è´¥é‡','æ€»é‚®ä»¶å‘é€å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(234,55,3,'sadmin','ä»Žvmem èŽ·å–æ—¥å¿—å†…å®¹å‡ºé”™','ä»Žvmem èŽ·å–æ—¥å¿—å†…å®¹å‡ºé”™',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(235,55,3,'sadmin','æ ¡éªŒvmem æ—¥å¿—å‡ºé”™','æ ¡éªŒvmem æ—¥å¿—å‡ºé”™',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(254,53,1,'sadmin','åˆ é™¤app æ—¥å¿—æ–‡ä»¶è¯·æ±‚é‡','åˆ é™¤app æ—¥å¿—æ–‡ä»¶è¯·æ±‚é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(255,53,1,'sadmin','æŸ¥è¯¢ app æ—¥å¿—æ–‡ä»¶å ç”¨ç©ºé—´è¯·æ±‚é‡','æŸ¥è¯¢ app æ—¥å¿—æ–‡ä»¶å ç”¨ç©ºé—´è¯·æ±‚é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(256,53,1,'sadmin','ååºåˆ—åŒ–è¯·æ±‚åŒ…å¤±è´¥','ååºåˆ—åŒ–è¯·æ±‚åŒ…å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(257,53,1,'sadmin','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´è¯·æ±‚é‡','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´è¯·æ±‚é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(258,53,1,'sadmin','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(259,53,1,'sadmin','å‘é€æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡','å‘é€æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(261,83,3,'sadmin','æµ‹è¯•äº‘ç‰ˆæœ¬å‘Šè­¦é€šé“','ç”¨äºŽæµ‹è¯•äº‘ç‰ˆæœ¬å‘Šè­¦é€šé“',0,1,1,0,'2020-04-08 08:43:58','2020-04-08 08:35:16'),(262,53,1,'sadmin','è®¾ç½®æ—¥å¿—æ–‡ä»¶åˆ é™¤æ ‡è®°é‡','è®¾ç½®æ—¥å¿—æ–‡ä»¶åˆ é™¤æ ‡è®°é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(263,55,1,'sadmin','é€šè¿‡åˆ é™¤æ ‡è®°åˆ é™¤æ—¥å¿—æ–‡ä»¶é‡','é€šè¿‡åˆ é™¤æ ‡è®°åˆ é™¤æ—¥å¿—æ–‡ä»¶é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(284,83,1,'sadmin','å½“å‰å‘Šè­¦äº§ç”Ÿé‡','å½“å‰å‘Šè­¦äº§ç”Ÿé‡',0,1,1,0,'2019-09-04 07:48:55','0000-00-00 00:00:00'),(285,55,1,'sadmin','æ—¥å¿—å†™å…¥ç£ç›˜é‡ï¼ˆå•ä½byteï¼‰','æ—¥å¿—å†™å…¥ç£ç›˜é‡  (å•ä½byteï¼‰',0,1,1,0,'2019-09-04 07:46:09','0000-00-00 00:00:00'),(330,53,5,'sadmin','è¿œç¨‹æ€»æ—¥å¿—è®°å½•é‡','agent client å‘é€è¿‡æ¥çš„æ—¥å¿—é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(331,53,5,'sadmin','ç›‘æŽ§ç³»ç»Ÿæœ¬èº«çš„æ—¥å¿—è®°å½•é‡','ç›‘æŽ§ç³»ç»Ÿæœ¬èº«çš„æ—¥å¿—è®°å½•é‡',0,1,1,0,'2019-08-31 11:26:58','0000-00-00 00:00:00'),(332,55,5,'sadmin','æ—¥å¿—è®°å½•å†™å…¥ç£ç›˜æ€»é‡','æ—¥å¿—æ€»é‡',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00'),(355,82,1,'sadmin','é‚®ä»¶å‘é€é‡','æ¯åˆ†é’Ÿçš„é‚®ä»¶å‘é€é‡',0,1,1,0,'2019-07-03 08:27:27','0000-00-00 00:00:00'),(356,82,5,'sadmin','é‚®ä»¶å‘é€é‡åŽ†å²æ€»é‡','æ€»çš„é‚®ä»¶å‘é€é‡',0,1,1,0,'2019-07-03 08:27:53','0000-00-00 00:00:00');
/*!40000 ALTER TABLE `mt_attr` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_ins after insert on mt_attr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_up after update on mt_attr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_attr_type`
--

DROP TABLE IF EXISTS `mt_attr_type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_attr_type` (
  `xrk_type` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `parent_type` int(11) unsigned DEFAULT '0',
  `type_pos` varchar(128) DEFAULT '0' COMMENT 'ç±»åž‹ä½ç½®',
  `xrk_name` varchar(64) NOT NULL,
  `attr_desc` varchar(256) DEFAULT NULL COMMENT 'ç±»åž‹æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `create_user` varchar(64) DEFAULT NULL,
  `mod_user` varchar(64) DEFAULT NULL,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_add_id` int(11) unsigned DEFAULT '0',
  `user_mod_id` int(11) unsigned DEFAULT '0',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`xrk_type`)
) ENGINE=InnoDB AUTO_INCREMENT=86 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_attr_type`
--

LOCK TABLES `mt_attr_type` WRITE;
/*!40000 ALTER TABLE `mt_attr_type` DISABLE KEYS */;
INSERT INTO `mt_attr_type` VALUES (1,0,'1','ç›‘æŽ§ç‚¹æ ¹ç±»åž‹','ç›‘æŽ§ç‚¹æ ¹ç±»åž‹',0,'sadmin','sadmin','2019-07-02 06:39:16',1,1,'0000-00-00 00:00:00'),(48,1,'1.1','ç›‘æŽ§ç³»ç»Ÿ','ç›‘æŽ§ç³»ç»Ÿè‡ªèº«çš„ç›¸å…³ç›‘æŽ§',0,'sadmin','sadmin','2020-03-18 14:03:13',1,1,'2020-03-18 13:26:01'),(50,48,'1.1.1','æ—¥å¿—ç›¸å…³','æ—¥å¿—ç›‘æŽ§ç›¸å…³æ¨¡å—çš„ç›‘æŽ§',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(51,50,'1.1.1.1','slog_client','æ—¥å¿—ç³»ç»Ÿå®¢æˆ·ç«¯ç›¸å…³ç›‘æŽ§ä¸ŠæŠ¥',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(53,50,'1.1.1.1','slog_serve','slog æ—¥å¿—æœåŠ¡å™¨ç«¯',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(54,50,'1.1.1.1','slog_config','è¯»å– mysql é…ç½®çš„è¿›ç¨‹',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(55,50,'1.1.1.1','slog_write','æ—¥å¿—æœåŠ¡å™¨ç«¯å†™æ—¥å¿—è¿›ç¨‹çš„ç›¸å…³ä¸ŠæŠ¥',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(56,48,'1.1.1','ç›‘æŽ§ç‚¹ç›¸å…³','ç›‘æŽ§ç‚¹ç›¸å…³æ¨¡å—',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(57,56,'1.1.1.1','monitor_client','ç›‘æŽ§ç³»ç»Ÿå®¢æˆ·ç«¯ monitor_client',0,'sadmin','sadmin','2020-03-18 13:54:57',1,1,'0000-00-00 00:00:00'),(58,56,'1.1.1.1','monitor_server','ç›‘æŽ§ç³»ç»ŸæœåŠ¡å™¨ç«¯ slog_monitor_server',0,'sadmin','sadmin','2020-03-18 13:54:57',1,1,'0000-00-00 00:00:00'),(66,48,'1.1.1','é€šç”¨ç»„ä»¶ä¸ŠæŠ¥','é€šç”¨ç»„ä»¶ç›‘æŽ§ä¸ŠæŠ¥',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(67,66,'1.1.1.1','vmem','vmem å¯å˜é•¿å…±äº«å†…å­˜ç»„ä»¶',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(68,48,'1.1.1','cgi ç›‘æŽ§','ç›‘æŽ§ç‚¹ç³»ç»Ÿ cgi/fcgi ç›¸å…³çš„ç›‘æŽ§ä¸ŠæŠ¥',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(69,68,'1.1.1.1','cgi å¼‚å¸¸ç›‘æŽ§','cgi å¼‚å¸¸ç‚¹ä¸ŠæŠ¥s',0,'sadmin','sadmin','2020-03-18 13:55:04',1,15,'0000-00-00 00:00:00'),(70,68,'1.1.1.1','cgi ä¸šåŠ¡é‡ç›‘æŽ§','cgi ä¸šåŠ¡é‡ç›‘æŽ§',0,'sadmin','sadmin','2020-03-18 13:55:04',1,1,'0000-00-00 00:00:00'),(71,66,'1.1.1.1','mysqlwrappe','mysql ç»„ä»¶',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(72,68,'1.1.1.1','cgi è°ƒç”¨è€—æ—¶ç›‘æŽ§','ç›‘æŽ§ç³»ç»Ÿ cgi è°ƒç”¨è€—æ—¶ç›‘æŽ§',0,'sadmin','sadmin','2020-03-18 13:55:04',1,1,'0000-00-00 00:00:00'),(80,66,'1.1.1.1','memcache','memcache ç›‘æŽ§',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(82,66,'1.1.1.1','mail','é‚®ä»¶å‘é€ç›‘æŽ§',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(83,56,'1.1.1.1','ç›‘æŽ§å‘Šè­¦ç›¸å…³','',0,'sadmin','sadmin','2020-03-18 13:54:57',1,1,'0000-00-00 00:00:00'),(84,1,'1.1','ç›‘æŽ§æ’ä»¶','æ‰€æœ‰ç›‘æŽ§æ’ä»¶ç›‘æŽ§ç‚¹ç±»åž‹çš„æ ¹èŠ‚ç‚¹',0,'sadmin','sadmin','2020-03-18 11:07:32',1,1,'2020-03-18 11:07:32');
/*!40000 ALTER TABLE `mt_attr_type` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_type_ins after insert on mt_attr_type for each row begin insert into mt_table_upate_monitor
(u_table_name, r_primary_id) values('mt_attr_type', new.xrk_type); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_type_up after update on mt_attr_type for each row begin insert into mt_table_upate_monitor(
u_table_name, r_primary_id) values('mt_attr_type', old.xrk_type); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_log_config`
--

DROP TABLE IF EXISTS `mt_log_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_log_config` (
  `app_id` int(11) unsigned DEFAULT '0',
  `module_id` int(11) unsigned DEFAULT '0',
  `log_types` int(10) unsigned DEFAULT '0',
  `config_name` varchar(32) NOT NULL,
  `config_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `create_time` int(12) unsigned DEFAULT '0',
  `user_add` varchar(64) NOT NULL,
  `user_mod` varchar(64) NOT NULL,
  `xrk_status` tinyint(4) DEFAULT '0',
  `config_desc` varchar(128) DEFAULT NULL,
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `write_speed` int(10) unsigned DEFAULT '0',
  PRIMARY KEY (`config_id`)
) ENGINE=InnoDB AUTO_INCREMENT=179 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_log_config`
--

LOCK TABLES `mt_log_config` WRITE;
/*!40000 ALTER TABLE `mt_log_config` DISABLE KEYS */;
INSERT INTO `mt_log_config` VALUES (30,26,127,'fcgi_mt_slog',34,'2019-08-26 12:47:52',0,'sadmin','sadmin',0,'fcgi_mt_slog',1,1,0),(30,25,127,'fcgi_slog_flogin',35,'2019-08-26 12:47:38',0,'sadmin','sadmin',0,'fcgi_slog_flogin',1,1,0),(30,24,127,'slog_client',36,'2019-08-26 12:47:25',0,'sadmin','sadmin',0,'slog_client',1,1,0),(30,23,127,'slog_write',37,'2019-08-26 12:47:15',0,'sadmin','sadmin',0,'slog_write',1,1,0),(30,22,127,'slog_server',38,'2019-08-26 12:47:03',0,'sadmin','sadmin',0,'slog_server',1,1,0),(30,21,127,'slog_config',39,'2019-08-26 12:46:54',0,'sadmin','sadmin',0,'slog_config',1,1,0),(30,27,127,'slog_mtreport_server',40,'2019-08-26 12:48:09',0,'sadmin','sadmin',0,'slog_mtreport_server',1,1,0),(30,28,127,'fcgi_mt_slog_monitor',41,'2019-08-26 12:46:43',0,'sadmin','sadmin',0,'cgi_monitor',1,1,0),(30,29,127,'fcgi_mt_slog_attr',42,'2019-08-26 12:46:29',0,'sadmin','sadmin',0,'mt_slog_attr',1,1,0),(30,30,127,'fcgi_mt_slog_machine',43,'2019-08-26 12:48:29',0,'sadmin','sadmin',0,'fcgi_mt_slog_machine',1,1,0),(30,31,127,'fcgi_mt_slog_view',44,'2019-08-26 12:49:26',0,'sadmin','sadmin',0,'fcgi_mt_slog_view',1,1,0),(30,33,127,'slog_monitor_server',46,'2019-08-26 12:45:58',0,'sadmin','sadmin',0,'config_monitor_server',1,1,0),(30,34,127,'fcgi_mt_slog_showview',47,'2019-08-26 12:49:43',0,'sadmin','sadmin',0,'fcgi_mt_slog_showview',1,1,0),(30,35,127,'fcgi_mt_slog_warn',48,'2019-08-31 11:06:51',0,'sadmin','sadmin',0,'fcgi_mt_slog_warn',1,1,0),(30,39,127,'fcgi_mt_slog_user',52,'2019-08-31 11:06:59',0,'sadmin','sadmin',0,'fcgi_mt_slog_user',1,1,222),(30,47,255,'slog_monitor_client',62,'2019-05-24 02:43:17',1501204510,'sadmin','sadmin',0,'server attr client',1,1,0),(30,33,127,'slog_monitor_server',63,'2019-08-27 00:14:59',1501219043,'sadmin','sadmin',0,' attr server',1,1,4200),(30,48,127,'slog_deal_warn',64,'2019-08-26 12:45:20',1502026252,'sadmin','sadmin',0,'mail',1,1,0),(30,59,127,'slog_check_warn',74,'2019-08-26 12:45:27',1533356355,'sadmin','sadmin',0,'å‘Šè­¦æ¨¡å—',1,1,0),(30,150,108,'fcgi_mt_slog_reportinfo',178,'2020-05-04 22:19:28',1588630768,'sadmin','sadmin',0,'å…¬å…±ä¸ŠæŠ¥cgi æ—¥å¿—é…ç½®',1,1,0);
/*!40000 ALTER TABLE `mt_log_config` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_log_config_ins after insert on mt_log_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_log_config', new.config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_log_config_up after update on mt_log_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_log_config', old.config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_machine`
--

DROP TABLE IF EXISTS `mt_machine`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_machine` (
  `xrk_id` int(11) NOT NULL AUTO_INCREMENT,
  `xrk_name` varchar(64) NOT NULL,
  `ip1` int(12) unsigned DEFAULT NULL,
  `ip2` int(12) unsigned DEFAULT NULL,
  `ip3` int(12) unsigned DEFAULT NULL,
  `ip4` int(12) unsigned DEFAULT NULL,
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `user_mod` varchar(64) DEFAULT 'sadmin' COMMENT 'æœ€åŽæ›´æ–°ç”¨æˆ·',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'æ›´æ–°æ—¶é—´',
  `machine_desc` varchar(256) DEFAULT NULL COMMENT 'æœºå™¨æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `warn_flag` int(8) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `model_id` int(8) DEFAULT '0' COMMENT 'æœºå™¨åž‹å·',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `last_attr_time` int(12) unsigned DEFAULT '0',
  `last_log_time` int(12) unsigned DEFAULT '0',
  `last_hello_time` int(12) unsigned DEFAULT '0',
  `start_time` int(12) unsigned DEFAULT '0',
  `cmp_time` varchar(32) DEFAULT NULL,
  `agent_version` varchar(20) DEFAULT NULL,
  `agent_os` varchar(32) DEFAULT NULL,
  `libc_ver` varchar(64) DEFAULT NULL,
  `libcpp_ver` varchar(64) DEFAULT NULL,
  `os_arc` varchar(64) DEFAULT NULL,
  PRIMARY KEY (`xrk_id`),
  KEY `ip1` (`ip1`)
) ENGINE=InnoDB AUTO_INCREMENT=127 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_machine`
--

LOCK TABLES `mt_machine` WRITE;
/*!40000 ALTER TABLE `mt_machine` DISABLE KEYS */;
INSERT INTO `mt_machine` VALUES (124,'comm_report',16777343,0,0,0,'sadmin','sadmin','2020-05-05 02:32:53','ç”¨äºŽæŽ¥æ”¶å…¬å…±ä¸ŠæŠ¥çš„è™šæ‹Ÿæœºå™¨',0,1,2,1,1,'2020-05-05 02:27:36',0,0,0,0,0,NULL,NULL,NULL,NULL,NULL),(126,'192.168.128.132',2223024320,NULL,NULL,NULL,'sadmin','sadmin','2020-09-10 08:06:08','ç³»ç»Ÿå®‰è£…æ—¶è‡ªåŠ¨æ·»åŠ ',0,0,0,1,1,'2020-09-10 08:06:08',0,0,0,0,0,NULL,NULL,NULL,NULL,NULL);
/*!40000 ALTER TABLE `mt_machine` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_machine_ins after insert on mt_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_machine', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_machine_up after update on mt_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_machine', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_module_info`
--

DROP TABLE IF EXISTS `mt_module_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_module_info` (
  `module_name` varchar(32) NOT NULL,
  `module_desc` varchar(128) NOT NULL,
  `app_id` int(11) unsigned DEFAULT '0',
  `module_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_add` varchar(32) NOT NULL,
  `user_mod` varchar(32) NOT NULL,
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`module_id`)
) ENGINE=InnoDB AUTO_INCREMENT=151 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_module_info`
--

LOCK TABLES `mt_module_info` WRITE;
/*!40000 ALTER TABLE `mt_module_info` DISABLE KEYS */;
INSERT INTO `mt_module_info` VALUES ('slog_config','å¤„ç†ç›‘æŽ§ç³»ç»Ÿç›¸å…³é…ç½®ï¼Œæœ‰æ›´æ–°æ—¶å°†é…ç½®ä»Žæ•°æ®åº“æ›´æ–°åˆ°å…±äº«å†…å­˜',30,21,0,'sadmin','sadmin','2019-08-26 12:50:24',1,1,'0000-00-00 00:00:00'),('slog_server','æ—¥å¿—æœåŠ¡ï¼ŒæŽ¥æ”¶è¿œç¨‹logå¹¶å†™å…¥æœåŠ¡å™¨å…±äº«å†…å­˜ä¸­',30,22,0,'sadmin','sadmin','2019-08-26 12:50:17',1,1,'0000-00-00 00:00:00'),('slog_write','æ—¥å¿—å†™å…¥ç£ç›˜æœåŠ¡',30,23,0,'sadmin','sadmin','2019-08-26 12:50:12',1,1,'0000-00-00 00:00:00'),('slog_client','ç”¨äºŽæ”¶é›†å¤šæœºéƒ¨ç½²æ—¶ï¼Œç›‘æŽ§ç³»ç»Ÿè‡ªèº«äº§ç”Ÿçš„æ—¥å¿—',30,24,0,'sadmin','sadmin','2019-08-26 12:50:07',1,1,'0000-00-00 00:00:00'),('fcgi_slog_flogin','ç”¨äºŽå¤„ç†ç”¨æˆ·ç™»å½•æŽˆæƒ',30,25,0,'sadmin','sadmin','2017-07-28 01:19:56',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog','ç”¨äºŽå¤„ç†ç³»ç»Ÿæ—¥å¿—å±•ç¤ºï¼Œåº”ç”¨æ¨¡å—é…ç½®ç­‰',30,26,0,'sadmin','sadmin','2014-12-14 04:04:29',1,1,'0000-00-00 00:00:00'),('slog_mtreport_server','ç®¡ç†ç›‘æŽ§ agent slog_mtreport_client çš„æŽ¥å…¥ï¼Œä»¥åŠä¸‹å‘é…ç½®',30,27,0,'sadmin','sadmin','2019-08-26 12:44:40',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_monitor','ç›‘æŽ§ç³»ç»Ÿä¸»é¡µ',30,28,0,'sadmin','sadmin','2019-07-02 07:53:44',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_attr','ç”¨äºŽç®¡ç†ç›‘æŽ§ç‚¹å’Œç›‘æŽ§ç‚¹ç±»åž‹',30,29,0,'sadmin','sadmin','2019-08-26 12:43:33',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_machine','ç”¨äºŽç®¡ç†ç³»ç»Ÿæœºå™¨é…ç½®',30,30,0,'sadmin','sadmin','2019-07-02 07:51:56',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_view','ç”¨äºŽå¤„ç†è§†å›¾é…ç½®',30,31,0,'sadmin','sadmin','2019-07-02 07:51:44',1,1,'0000-00-00 00:00:00'),('slog_monitor_server','ç”¨äºŽå¤„ç†ç›‘æŽ§ç‚¹ä¸ŠæŠ¥',30,33,0,'sadmin','sadmin','2019-08-26 12:12:09',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_showview','å¤„ç†webç³»ç»Ÿè§†å›¾å±•ç¤º',30,34,0,'sadmin','sadmin','2018-05-23 11:11:49',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_warn','å‘Šè­¦é…ç½®',30,35,0,'sadmin','sadmin','2019-08-31 11:07:39',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_user','ç›‘æŽ§ç³»ç»Ÿç”¨æˆ·ç®¡ç†cgi',30,39,0,'sadmin','sadmin','2019-08-31 11:07:26',1,1,'0000-00-00 00:00:00'),('slog_monitor_client','ç›‘æŽ§ç³»ç»Ÿæœ¬èº«çš„ç›‘æŽ§ç‚¹ä¸ŠæŠ¥æœåŠ¡',30,47,0,'sadmin','sadmin','2019-08-26 12:11:46',1,1,'0000-00-00 00:00:00'),('slog_deal_warn','ç›‘æŽ§å‘Šè­¦å¤„ç†æ¨¡å—',30,48,0,'sadmin','sadmin','2019-08-26 12:11:00',1,1,'0000-00-00 00:00:00'),('slog_check_warn','å‘Šè­¦æ£€æŸ¥æ¨¡å—',30,59,0,'sadmin','sadmin','2019-08-26 12:11:26',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_reportinfo','ç”¨äºŽå…¬å…±ä¸ŠæŠ¥çš„ fastcgi æ¨¡å—',30,150,0,'sadmin','sadmin','2020-05-04 22:18:17',1,1,'2020-05-04 22:18:17');
/*!40000 ALTER TABLE `mt_module_info` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_module_info_ins after insert on mt_module_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_module_info', new.module_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_module_info_up after update on mt_module_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_module_info', old.module_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_plugin`
--

DROP TABLE IF EXISTS `mt_plugin`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_plugin` (
  `plugin_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `xrk_status` tinyint(4) DEFAULT '0',
  `create_time` int(12) unsigned DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT '1970-01-01 16:00:00' ON UPDATE CURRENT_TIMESTAMP,
  `pb_info` text,
  `open_plugin_id` int(11) unsigned NOT NULL,
  `plugin_name` varchar(64) NOT NULL,
  `plugin_show_name` varchar(128) NOT NULL,
  PRIMARY KEY (`plugin_id`),
  KEY `open_id_key` (`open_plugin_id`)
) ENGINE=InnoDB AUTO_INCREMENT=72 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_plugin`
--

LOCK TABLES `mt_plugin` WRITE;
/*!40000 ALTER TABLE `mt_plugin` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_plugin` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `mt_plugin_machine`
--

DROP TABLE IF EXISTS `mt_plugin_machine`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_plugin_machine` (
  `xrk_id` int(11) NOT NULL AUTO_INCREMENT,
  `machine_id` int(11) NOT NULL,
  `last_attr_time` int(12) unsigned DEFAULT '0',
  `last_log_time` int(12) unsigned DEFAULT '0',
  `start_time` int(12) unsigned DEFAULT '0',
  `lib_ver_num` int(11) DEFAULT '0',
  `cfg_version` char(12) NOT NULL,
  `build_version` char(12) NOT NULL,
  `last_hello_time` int(12) unsigned DEFAULT '0',
  `xrk_status` int(11) DEFAULT '0',
  `install_proc` int(11) DEFAULT '0',
  `open_plugin_id` int(11) unsigned NOT NULL,
  `local_cfg_url` varchar(1024) NOT NULL,
  PRIMARY KEY (`xrk_id`),
  UNIQUE KEY `m_p` (`machine_id`)
) ENGINE=InnoDB AUTO_INCREMENT=193 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_plugin_machine`
--

LOCK TABLES `mt_plugin_machine` WRITE;
/*!40000 ALTER TABLE `mt_plugin_machine` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_plugin_machine` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_plugin_machine_ins after insert on mt_plugin_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_plugin_machine', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_plugin_machine_up after update on mt_plugin_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_plugin_machine', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_server`
--

DROP TABLE IF EXISTS `mt_server`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_server` (
  `ip` char(20) NOT NULL,
  `xrk_port` int(11) unsigned NOT NULL DEFAULT '12345',
  `xrk_type` int(8) unsigned NOT NULL DEFAULT '0',
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `sand_box` int(8) unsigned DEFAULT '0',
  `region` int(8) unsigned DEFAULT '0',
  `idc` int(8) unsigned DEFAULT '0',
  `xrk_status` tinyint(4) DEFAULT '0',
  `srv_for` text,
  `weight` int(11) unsigned DEFAULT '60000',
  `cfg_seq` int(11) unsigned DEFAULT '1',
  `user_add` varchar(64) NOT NULL,
  `user_mod` varchar(64) NOT NULL,
  `create_time` int(12) unsigned DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `m_desc` varchar(256) DEFAULT NULL,
  PRIMARY KEY (`ip`,`xrk_type`),
  KEY `id` (`xrk_id`)
) ENGINE=InnoDB AUTO_INCREMENT=25 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_server`
--

LOCK TABLES `mt_server` WRITE;
/*!40000 ALTER TABLE `mt_server` DISABLE KEYS */;
INSERT INTO `mt_server` VALUES ('192.168.128.132',28080,1,3,0,0,0,0,'30,119',1000,1584529976,'sadmin','sadmin',1474811820,'2020-09-10 08:06:08','ç»‘å®šåº”ç”¨idï¼Œå¤„ç†æ—¥å¿—ä¸ŠæŠ¥ï¼Œå¯éƒ¨ç½²å¤šå°'),('192.168.128.132',38080,2,6,0,0,0,0,'',1000,1567504630,'sadmin','sadmin',1475408172,'2020-09-10 08:06:08','å¤„ç†ç›‘æŽ§ç‚¹æ•°æ®ä¸ŠæŠ¥ã€å¯éƒ¨ç½²å¤šå°'),('192.168.128.132',3306,3,4,0,0,0,0,'',1000,1567577833,'sadmin','sadmin',1475152894,'2020-09-10 08:06:08','mysql ç›‘æŽ§ç‚¹æœåŠ¡å™¨ï¼Œéƒ¨ç½²1å°'),('192.168.128.132',27000,4,24,0,0,0,0,'ä¸­å¿ƒæœåŠ¡å™¨ç”¨äºŽå½’æ€»å„æœåŠ¡å™¨éƒ½æœ‰ä¸ŠæŠ¥çš„æ•°æ®ï¼Œå±žäºŽ slog_mtreport_server ä¸­çš„æŸä¸€å°',1000,1589376081,'sadmin','sadmin',0,'2020-09-10 08:06:08','slog_mtreport_server ä¸­çš„ä¸€å°'),('192.168.128.132',12121,11,23,0,0,0,0,'',1000,1567504613,'sadmin','sadmin',1561962711,'2020-09-10 08:06:08','web æŽ§åˆ¶å°æœåŠ¡å™¨ï¼Œéƒ¨ç½²1å°');
/*!40000 ALTER TABLE `mt_server` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_server_ins after insert on mt_server for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_server', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_server_up after update on mt_server for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_server', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_table_upate_monitor`
--

DROP TABLE IF EXISTS `mt_table_upate_monitor`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_table_upate_monitor` (
  `u_table_name` varchar(32) NOT NULL,
  `r_primary_id` int(11) unsigned DEFAULT '0',
  `r_primary_id_2` int(11) unsigned DEFAULT '0',
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `r_change_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`r_change_id`)
) ENGINE=MyISAM AUTO_INCREMENT=40040 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_table_upate_monitor`
--

LOCK TABLES `mt_table_upate_monitor` WRITE;
/*!40000 ALTER TABLE `mt_table_upate_monitor` DISABLE KEYS */;
INSERT INTO `mt_table_upate_monitor` VALUES ('mt_server',3,0,'2020-09-10 08:06:08',40030),('mt_server',6,0,'2020-09-10 08:06:08',40031),('mt_server',4,0,'2020-09-10 08:06:08',40032),('mt_server',24,0,'2020-09-10 08:06:08',40033),('mt_server',23,0,'2020-09-10 08:06:08',40034),('mt_machine',126,0,'2020-09-10 08:06:08',40035),('mt_view_bmach',23,126,'2020-09-10 08:07:00',40036),('mt_view_bmach',22,126,'2020-09-10 08:07:00',40037),('flogin_user',1,0,'2020-09-10 08:07:53',40038),('mt_view_bmach',26,126,'2020-09-10 08:08:00',40039);
/*!40000 ALTER TABLE `mt_table_upate_monitor` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `mt_view`
--

DROP TABLE IF EXISTS `mt_view`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_view` (
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `xrk_name` varchar(64) NOT NULL,
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `user_mod` varchar(64) DEFAULT 'sadmin' COMMENT 'æœ€åŽæ›´æ–°ç”¨æˆ·',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'æ›´æ–°æ—¶é—´',
  `view_desc` varchar(256) DEFAULT NULL COMMENT 'è§†å›¾æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `view_flag` int(8) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`xrk_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1000032 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view`
--

LOCK TABLES `mt_view` WRITE;
/*!40000 ALTER TABLE `mt_view` DISABLE KEYS */;
INSERT INTO `mt_view` VALUES (22,'ç›‘æŽ§ç³»ç»Ÿ-å¸¸ç”¨','sadmin','sadmin','2019-05-23 03:17:30','ç›‘æŽ§ç³»ç»Ÿå¸¸ç”¨ä¸ŠæŠ¥',0,1,1,1,'0000-00-00 00:00:00'),(23,'æ—¥å¿—ç³»ç»Ÿ-æ•°æ®','sadmin','sadmin','2019-07-03 03:43:57','ä¸€äº›é‡è¦çš„æ—¥å¿—ç³»ç»Ÿæ•°æ®å±žæ€§ä¸ŠæŠ¥',0,1,1,1,'0000-00-00 00:00:00'),(26,'cgi ç›‘æŽ§','sadmin','sadmin','2019-05-23 03:17:00','cgiç›¸å…³ç›‘æŽ§ä¸ŠæŠ¥',0,1,1,1,'0000-00-00 00:00:00');
/*!40000 ALTER TABLE `mt_view` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_ins after insert on mt_view for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_view', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_up after update on mt_view for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_view', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_view_battr`
--

DROP TABLE IF EXISTS `mt_view_battr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_view_battr` (
  `view_id` int(11) NOT NULL DEFAULT '0' COMMENT 'è§†å›¾ç¼–å·',
  `attr_id` int(11) NOT NULL DEFAULT '0' COMMENT 'å±žæ€§ç¼–å·',
  `xrk_status` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`view_id`,`attr_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view_battr`
--

LOCK TABLES `mt_view_battr` WRITE;
/*!40000 ALTER TABLE `mt_view_battr` DISABLE KEYS */;
INSERT INTO `mt_view_battr` VALUES (21,62,0,'2019-11-17 11:34:44'),(21,74,0,'2019-11-17 11:34:44'),(21,63,0,'2019-11-17 11:34:44'),(21,56,0,'2019-11-17 11:34:44'),(21,55,0,'2019-11-17 11:34:44'),(21,57,0,'2019-11-17 11:34:44'),(21,64,0,'2019-11-17 11:34:44'),(21,71,0,'2019-11-17 11:34:44'),(21,73,0,'2019-11-17 11:34:44'),(21,58,0,'2019-11-17 11:34:44'),(21,65,0,'2019-11-17 11:34:44'),(23,70,0,'2019-11-17 11:34:44'),(26,113,0,'2019-11-17 11:34:44'),(23,65,0,'2019-11-17 11:34:44'),(23,93,0,'2019-11-17 11:34:44'),(21,67,0,'2019-11-17 11:34:44'),(21,75,0,'2019-11-17 11:34:44'),(21,59,0,'2019-11-17 11:34:44'),(21,60,0,'2019-11-17 11:34:44'),(21,70,0,'2019-11-17 11:34:44'),(23,73,0,'2019-11-17 11:34:44'),(23,75,0,'2019-11-17 11:34:44'),(21,61,0,'2019-11-17 11:34:44'),(21,66,0,'2019-11-17 11:34:44'),(21,68,0,'2019-11-17 11:34:44'),(21,69,0,'2019-11-17 11:34:44'),(21,72,0,'2019-11-17 11:34:44'),(21,76,0,'2019-11-17 11:34:44'),(23,67,0,'2019-11-17 11:34:44'),(21,77,0,'2019-11-17 11:34:44'),(21,78,0,'2019-11-17 11:34:44'),(21,79,0,'2019-11-17 11:34:44'),(21,80,0,'2019-11-17 11:34:44'),(21,81,0,'2019-11-17 11:34:44'),(21,82,0,'2019-11-17 11:34:44'),(21,83,0,'2019-11-17 11:34:44'),(26,143,0,'2019-11-17 11:34:44'),(26,112,0,'2019-11-17 11:34:44'),(26,111,0,'2019-11-17 11:34:44'),(26,110,0,'2019-11-17 11:34:44'),(26,146,0,'2019-11-17 11:34:44'),(26,147,0,'2019-11-17 11:34:44'),(26,145,0,'2019-11-17 11:34:44'),(26,108,0,'2019-11-17 11:34:44'),(26,144,0,'2019-11-17 11:34:44'),(26,142,0,'2019-11-17 11:34:44'),(23,62,0,'2019-11-17 11:34:44'),(23,94,0,'2019-11-17 11:34:44'),(22,67,0,'2019-11-17 11:34:44'),(22,73,0,'2019-11-17 11:34:44');
/*!40000 ALTER TABLE `mt_view_battr` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_battr_ins after insert on mt_view_battr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_battr', new.view_id, new.attr_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_battr_up after update on mt_view_battr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_battr', old.view_id, old.attr_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_view_bmach`
--

DROP TABLE IF EXISTS `mt_view_bmach`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_view_bmach` (
  `view_id` int(11) NOT NULL DEFAULT '0' COMMENT 'è§†å›¾ç¼–å·',
  `machine_id` int(11) NOT NULL DEFAULT '0' COMMENT 'æœºå™¨ç¼–å·',
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_master` int(11) unsigned DEFAULT '1',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`view_id`,`machine_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view_bmach`
--

LOCK TABLES `mt_view_bmach` WRITE;
/*!40000 ALTER TABLE `mt_view_bmach` DISABLE KEYS */;
INSERT INTO `mt_view_bmach` VALUES (23,126,0,1,'2020-09-10 08:07:00'),(22,126,0,1,'2020-09-10 08:07:00'),(26,126,0,1,'2020-09-10 08:08:00');
/*!40000 ALTER TABLE `mt_view_bmach` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_bmach_ins after insert on mt_view_bmach for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_bmach', new.view_id, new.machine_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_bmach_up after update on mt_view_bmach for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_bmach', old.view_id, old.machine_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_warn_config`
--

DROP TABLE IF EXISTS `mt_warn_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_warn_config` (
  `warn_config_id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'å‘Šè­¦é…ç½®ç¼–å·',
  `attr_id` int(11) DEFAULT NULL COMMENT 'å±žæ€§ç¼–å·',
  `warn_flag` int(11) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `max_value` int(11) DEFAULT '0' COMMENT 'æœ€å¤§å€¼',
  `min_value` int(11) DEFAULT '0' COMMENT 'æœ€å°å€¼',
  `wave_value` int(11) DEFAULT '0' COMMENT 'æ³¢åŠ¨å€¼',
  `warn_type_value` int(11) DEFAULT '0' COMMENT 'å‘Šè­¦ç±»åž‹å€¼',
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `reserved1` int(11) DEFAULT '0' COMMENT 'æœºå™¨id æˆ–è€… è§†å›¾id',
  `reserved2` int(11) DEFAULT '0' COMMENT 'ä¿ç•™',
  `reserved3` varchar(32) DEFAULT NULL COMMENT 'ä¿ç•™',
  `reserved4` varchar(32) DEFAULT NULL COMMENT 'ä¿ç•™',
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`warn_config_id`)
) ENGINE=MyISAM AUTO_INCREMENT=120 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_warn_config`
--

LOCK TABLES `mt_warn_config` WRITE;
/*!40000 ALTER TABLE `mt_warn_config` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_warn_config` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_config_ins after insert on mt_warn_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_config', new.warn_config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_config_up after update on mt_warn_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_config', old.warn_config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `mt_warn_info`
--

DROP TABLE IF EXISTS `mt_warn_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_warn_info` (
  `wid` int(11) NOT NULL AUTO_INCREMENT,
  `warn_id` int(11) DEFAULT '0',
  `attr_id` int(11) DEFAULT '0',
  `warn_config_val` int(11) DEFAULT '0',
  `warn_val` int(11) DEFAULT '0',
  `warn_time_utc` int(11) unsigned DEFAULT NULL,
  `warn_flag` int(11) DEFAULT '0',
  `deal_status` int(11) DEFAULT '0',
  `last_warn_time_utc` int(11) unsigned DEFAULT NULL,
  `warn_times` int(11) DEFAULT '0',
  `send_warn_times` int(11) DEFAULT '0',
  `start_deal_time_utc` int(11) unsigned DEFAULT NULL,
  `end_deal_time_utc` int(11) unsigned DEFAULT NULL,
  `xrk_status` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`wid`)
) ENGINE=InnoDB AUTO_INCREMENT=6656 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_warn_info`
--

LOCK TABLES `mt_warn_info` WRITE;
/*!40000 ALTER TABLE `mt_warn_info` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_warn_info` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_info_ins after insert on mt_warn_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_info', new.wid); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_info_up after update on mt_warn_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_info', old.wid); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;

--
-- Table structure for table `test_key_list`
--

DROP TABLE IF EXISTS `test_key_list`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `test_key_list` (
  `config_id` int(10) unsigned NOT NULL DEFAULT '0',
  `test_key` varchar(128) NOT NULL,
  `test_key_type` int(10) unsigned DEFAULT '0',
  `xrk_status` tinyint(4) DEFAULT '0',
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`xrk_id`),
  KEY `config_id` (`config_id`)
) ENGINE=InnoDB AUTO_INCREMENT=40 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `test_key_list`
--

LOCK TABLES `test_key_list` WRITE;
/*!40000 ALTER TABLE `test_key_list` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_key_list` ENABLE KEYS */;
UNLOCK TABLES;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger test_key_list_ins after insert on test_key_list for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('test_key_list', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger test_key_list_up after update on test_key_list for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('test_key_list', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2020-09-10 17:34:11

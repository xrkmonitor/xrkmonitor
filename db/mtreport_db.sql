-- MySQL dump 10.13  Distrib 5.7.27, for Linux (x86_64)
--
-- Host: localhost    Database: mtreport_db
-- ------------------------------------------------------
-- Server version	5.7.27-0ubuntu0.18.04.1

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

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `mtreport_db` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `mtreport_db`;

--
-- Table structure for table `flogin_history`
--

DROP TABLE IF EXISTS `flogin_history`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `flogin_history` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(11) unsigned NOT NULL,
  `login_time` int(12) unsigned DEFAULT '0',
  `login_remote_address` char(16) DEFAULT '0.0.0.0',
  `receive_server` char(16) DEFAULT '0.0.0.0',
  `referer` varchar(1024) DEFAULT NULL,
  `user_agent` varchar(1024) DEFAULT NULL,
  `method` int(10) DEFAULT '0',
  `valid_time` int(11) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `user_id` (`user_id`)
) ENGINE=MyISAM AUTO_INCREMENT=294 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `flogin_history`
--

LOCK TABLES `flogin_history` WRITE;
/*!40000 ALTER TABLE `flogin_history` DISABLE KEYS */;
INSERT INTO `flogin_history` VALUES (192,79,1555046058,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555046044','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',1,1555049658),(291,1,1561005408,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1561005400','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561610208),(245,23,1558314326,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1558314317','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',1,1558919126),(288,1,1561004777,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1561004770','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561609577),(63,79,1550991721,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/mt_slog_monitor','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',1,1550995321),(118,84,1552821811,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=iframe_login','Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.87 Safari/537.36',4,1553426611),(96,85,1552385131,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',8,1552989931),(241,23,1557882649,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1557882641','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1558487449),(187,104,1554880086,'192.168.128.2','192.168.128.128','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554880067','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555484886),(292,1,1561018379,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1561018368','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561623179),(159,100,1554637088,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554637085','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555241888),(91,84,1552375851,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',8,1552980651),(244,23,1558314216,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1558314207','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1558919016),(38,81,1550391645,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36',1,1550395245),(253,23,1559013977,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1559013967','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1559618777),(126,86,1553329221,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=iframe_login','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',1,1553934021),(58,79,1550971531,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin?redirect_url=/cgi-bin/mt_slog_monitor','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',1,1550975131),(59,79,1550975404,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin?action=logout&redirect_url=http://xrkmonitor.com/cgi-bin/mt_slog_monitor','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',1,1550979004),(201,105,1555294270,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555294265','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555899070),(278,23,1559787531,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1559787523','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1560392331),(78,79,1551410213,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36',1,1551413813),(79,79,1551418505,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36',1,1551422105),(80,79,1551423613,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36',1,1551427213),(152,100,1554620962,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555225762),(285,1,1560845808,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560845798','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561450608),(125,86,1553329004,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553933804),(77,79,1551403011,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36',1,1551406611),(195,79,1555051823,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555051810','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',1,1555055423),(286,1,1560931576,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560931567','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561536376),(56,79,1550919936,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/slog_flogin?redirect_url=/cgi-bin/mt_slog_monitor','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',1,1550923536),(57,79,1550963726,'192.168.128.1','192.168.128.110','http://xrkmonitor.com/cgi-bin/mt_slog_monitor','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',1,1550967326),(197,105,1555288440,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555288436','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555893240),(290,1,1561005207,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1561005199','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561610007),(281,23,1560147808,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560147796','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1560752608),(205,106,1555295697,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555295690','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555900497),(235,23,1557469028,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1557469017','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1558073828),(179,102,1554798143,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554798132','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',1,1555402943),(194,79,1555048673,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555048654','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',1,1555052273),(151,100,1554554062,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=iframe_login&v=1554553667','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555158862),(127,87,1553332223,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',8,1553937023),(128,88,1553332258,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553937058),(129,89,1553333128,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553937928),(130,90,1553333130,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553937930),(131,91,1553333135,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553937935),(132,92,1553333143,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553937943),(133,93,1553333367,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553938167),(134,94,1553334062,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.87 Safari/537.36',8,1553938862),(135,95,1553334154,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553938954),(136,96,1553334709,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553939509),(137,97,1553334912,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko',8,1553939712),(138,99,1553335275,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=register','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:65.0) Gecko/20100101 Firefox/65.0',8,1553940075),(139,94,1553335897,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=iframe_login','Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.87 Safari/537.36',1,1553940697),(153,100,1554622563,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554622552','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555227363),(154,100,1554622796,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554622791','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555227596),(155,100,1554622943,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=iframe_login&v=','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555227743),(156,100,1554623317,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554623301','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555228117),(157,100,1554626980,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554626947','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555231780),(279,1,1560131099,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560131091','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1560735899),(160,100,1554640235,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554637128','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555245035),(161,100,1554640396,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554640361','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555245196),(162,100,1554640435,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554640433','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555245235),(176,102,1554770058,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554769854','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555374858),(177,102,1554780188,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554780174','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555384988),(237,23,1557715054,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1557715044','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1558319854),(180,102,1554798532,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554798523','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',1,1555403332),(181,102,1554799085,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554799077','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',1,1555403885),(182,103,1554856387,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=iframe_login&v=','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555461187),(183,104,1554856513,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=main_login&v=1554856454','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555461313),(169,102,1554646123,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554646122','Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',6,1555250923),(170,102,1554646211,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554646207','Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36 SE 2.X MetaSr 1.0',6,1555251011),(171,102,1554646268,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554646262','Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 Safari/537.36',6,1555251068),(172,102,1554646374,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554646368','Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.104 Safari/537.36 Core/1.53.2372.400 QQBrowser/9.5.11096.400',6,1555251174),(173,102,1554646475,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554646473','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555251275),(174,102,1554646533,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554646530','Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555251333),(175,102,1554702924,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554702651','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555307724),(184,104,1554857503,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554857498','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555462303),(185,104,1554868438,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554868432','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555473238),(284,1,1560820483,'192.168.128.2','192.168.128.110','http://192.168.128.110/cgi-bin/slog_flogin?action=iframe_login&v=1560820473','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561425283),(188,104,1554880637,'192.168.128.2','192.168.128.128','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1554880632','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555485437),(239,23,1557816127,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1557816117','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1558420927),(289,1,1561004882,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1561004874','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561609682),(287,1,1560935675,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560935666','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561540475),(206,105,1555295722,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555295716','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555900522),(207,107,1555295781,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555295742','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555900581),(208,107,1555297366,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1555297362','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:66.0) Gecko/20100101 Firefox/66.0',6,1555902166),(229,23,1557205972,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1557205961','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1557810772),(282,23,1560305467,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560305461','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',6,1560910267),(283,23,1560307846,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560307837','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',6,1560912646),(293,1,1561084961,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1561084953','Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0',1,1561689761),(280,1,1560147693,'192.168.128.2','192.168.128.110','http://local.xrkmonitor.com/cgi-bin/slog_flogin?action=iframe_login&v=1560147683','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36',1,1560752493);
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
  `status` int(11) DEFAULT '0',
  `email` varchar(64) DEFAULT NULL,
  `login_index` int(11) DEFAULT '0',
  `login_md5` varchar(32) DEFAULT '',
  `last_login_server` char(16) DEFAULT '0.0.0.0',
  `bind_xrkmonitor_uid` int(11) unsigned DEFAULT '0',
  PRIMARY KEY (`user_id`),
  KEY `idx_name` (`user_name`)
) ENGINE=InnoDB AUTO_INCREMENT=150 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `flogin_user`
--

LOCK TABLES `flogin_user` WRITE;
/*!40000 ALTER TABLE `flogin_user` DISABLE KEYS */;
INSERT INTO `flogin_user` VALUES (1,'sadmin','7fb58ce340e389dc80847f8f1c77d0ca',1,'2019-09-03 09:57:46','supperuser',1552345835,152,NULL,'0c96aa43ce31757758c40be976c20d34',1,0,0,'115.44.234.97',1,1,0,'4033@qq.com',0,'','',0),(37,'mtreport','367872935bbb5c2d218b76ef7c2d7096',0,'2019-09-03 00:46:31','mtreport',0,0,'','',1,1567471591,1545918635,'115.44.234.97',1,1,0,'',11,'367872935bbb5c2d218b76ef7c2d7096','172.18.67.243',0),(109,'testadd','c099ca4b61b8845d7ba362c855e0fcc0',0,'2019-09-03 03:37:24','dsds12233',0,0,NULL,NULL,0,0,1562035027,'115.44.234.97',1,1,0,NULL,0,'','',0),(110,'EtSvznN','2ebbb1061f720a47a54b7a1966821708',1,'2019-09-03 07:56:14','',0,0,NULL,NULL,0,0,1567481215,'115.44.234.97',1,1,0,NULL,0,'','',0),(111,'RQaAsfrpRB','d1af092ae0cd75e1d51454959dc30dc1',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481225,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(112,'ZZaUd8q','2030b4cfd8d42f4bde045da3c30c811d',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481236,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(113,'pIPSrqaCB','9810d87d79c02e37e87b8daf34211ea0',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481246,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(114,'fSy5qIcRT','3dd4017ee47594ec992789fc692866cd',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481257,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(115,'usg4ZNvVvWB','6d00263b52b9f1ce60bb919e6d2ff2b0',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481268,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(116,'DdjdexgM6c','300cb0249f618f8ba530a5f86fc6e26d',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481276,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(117,'DK9XNGfU4','8424da8f669ef71b8bcb38f44fc63cef',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481286,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(118,'vrJhQnCX8X','622a457592a8d62f67bcd3bbdeaecc75',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481295,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(119,'mBWqaQ','4bdb7e4028a2c4e2a99f9510afaf5d91',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481304,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(120,'eYh4n6Prh','5e46184bd9b0fd0a53ab09c799b5b025',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481331,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(121,'mCmuJxua98','e3805d08ecc176bd89d823f5610a72f7',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481338,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(122,'a2vu8cdR','9898767ebf4cb66ee04d40cc8c1dc235',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481345,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(123,'qUyn4a56MaU','dff74b6ed63f6dbb95622784af9a7c33',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481352,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(124,'DbRuhTy4tqg','d2808fc5c047d6ec950bfd9b8c545e5d',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481359,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(125,'aznuuNi8E8X','a83a1e4392884ef93b49a5b95ae39891',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481366,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(126,'kCNQbS4NDaK','0e24a5d1813a814af520c95239f4489c',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481373,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(127,'S4sHI7T','40dc1f5c212fbf39e45a335afe4b7b15',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481379,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(128,'WXI4E4V8','fb9e0069733b5173ec51ad1b849c1c98',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481387,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(129,'EhTBHgI2g','151bdc25346b63f536a67ae127f44a13',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481393,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(130,'d9MweuWV','c075b9f33b91a933544d9f52d8b28e25',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481427,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(131,'Ta66JJDj5k8','90ee79bc55d81f5f02fff56ce0cc63b5',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481436,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(132,'bqmfIxPvCB','ff81ffee229ec54f0002fc750c26e5ed',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481443,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(133,'zQjaFrpZyRu','8ce9b9dd499b6545b16882a6962bcf43',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481450,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(134,'zfvHMCvjn','9c9f7d7cd378aa9d7ee349cf4ccb3274',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481456,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(135,'xWFW2ifmC94','3fd1322c56ce9ad62f2b55d4b95fdfc6',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481462,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(136,'FNzNJ74ghZR','aab00501cfa5730711111749e18f6441',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481470,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(137,'jWChyqYT2','3bcce913ae0d17c47d919f0076320d9d',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481476,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(138,'d8VwcBq2nW','dc167bb33b88e7cb90456bd09d4b6462',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481483,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(139,'kgmUYN','d35b2d31661835e739af22288e5c52a1',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481491,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(140,'ZzdhPDb','02565384bed62ab3f2ce5123cbb0e909',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481618,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(141,'TDhv4HhR','1ac96e0ea3f35116343389e5d0206090',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481624,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(142,'nEXnZhQm','8ae6c721572784211147d5aa8365c1d2',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481632,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(143,'yeSWU3JyADu','e5fab2ef5b85eb406bfaa269e6845966',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481638,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(144,'VzjPQe','23231f3c88d8e91c1964f722f3dffda5',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481645,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(145,'PINYqq8','8884d26e3a2def78cceec36a773f141b',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481652,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(146,'ZbiEaCHAD','1e6ec33092476b106ac9b40022e39fa8',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481659,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(147,'mFSna5CBHHS','cb9f8b3af8e7fe0a9210ec4ba880d0de',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481664,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(148,'iN4eRFXwr','729fdc4e8cd247ef376a1275634f68e5',1,'2019-09-03 03:37:10','',0,0,NULL,NULL,0,0,1567481670,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0),(149,'KxuAvz','af9dab38d744a8bceb018bc137be7f48',0,'0000-00-00 00:00:00','',0,0,NULL,NULL,0,0,1567481677,'0.0.0.0',1,1,0,NULL,0,'','0.0.0.0',0);
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
  `status` tinyint(4) DEFAULT '0',
  `app_type` int(10) unsigned DEFAULT '2',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  PRIMARY KEY (`app_id`)
) ENGINE=InnoDB AUTO_INCREMENT=119 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_app_info`
--

LOCK TABLES `mt_app_info` WRITE;
/*!40000 ALTER TABLE `mt_app_info` DISABLE KEYS */;
INSERT INTO `mt_app_info` VALUES ('ç›‘æŽ§ç³»ç»Ÿ','ç›‘æŽ§ç³»ç»Ÿ',30,0,'2019-01-25 23:42:34','rockdeng','rockdeng',0,3,1,1);
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
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'å±žæ€§ç¼–å·',
  `attr_type` int(8) DEFAULT '0' COMMENT 'å±žæ€§ç±»åž‹',
  `data_type` int(6) DEFAULT '0' COMMENT 'æ•°æ®ç±»åž‹',
  `user_add` varchar(64) DEFAULT 'rock' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'æ·»åŠ æ—¶é—´',
  `attr_name` varchar(64) NOT NULL COMMENT 'å±žæ€§åç§°',
  `attr_desc` varchar(256) DEFAULT NULL COMMENT 'å±žæ€§æè¿°',
  `status` tinyint(4) DEFAULT '0' COMMENT 'çŠ¶æ€ç  0-æ­£å¸¸ä½¿ç”¨ 1-åˆ é™¤æ ‡å¿—',
  `user_add_id` int(11) unsigned DEFAULT '0',
  `user_mod_id` int(11) unsigned DEFAULT '0',
  `excep_attr_mask` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `str_attr_type` tinyint(4) DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=357 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_attr`
--

LOCK TABLES `mt_attr` WRITE;
/*!40000 ALTER TABLE `mt_attr` DISABLE KEYS */;
INSERT INTO `mt_attr` VALUES (55,79,1,'rockdeng','2014-01-25 05:58:24','å…¨éƒ¨ç½‘å¡ æ€»å…¥åŒ…é‡','å…¨éƒ¨ç½‘å¡ æ€»å…¥åŒ…é‡',0,1,1,0,'2019-08-28 09:04:22',0),(56,79,1,'rockdeng','2014-01-25 06:06:38','å…¨éƒ¨ç½‘å¡ æ€»å‡ºåŒ…é‡','å…¨éƒ¨ç½‘å¡ æ€»å‡ºåŒ…é‡',0,1,1,0,'2019-08-28 09:04:45',0),(57,79,1,'rockdeng','2014-01-25 06:38:44','å…¨éƒ¨ç½‘å¡ æ€»å…¥æµé‡','å…¨éƒ¨ç½‘å¡ æ€»å…¥æµé‡',0,1,1,0,'2019-08-28 09:05:06',0),(58,79,1,'rockdeng','2014-01-25 06:39:15','å…¨éƒ¨ç½‘å¡ æ€»å‡ºæµé‡','å…¨éƒ¨ç½‘å¡ æ€»å‡ºæµé‡',0,1,1,0,'2019-08-28 09:05:32',0),(59,79,1,'rockdeng','2014-01-25 06:55:49','ç½‘å¡0 å…¥åŒ…é‡','ç½‘å¡0 ç½‘å¡åä¸æ˜¯eth0 æ—¶ä½¿ç”¨',0,1,1,0,'2019-08-28 09:06:53',0),(60,79,1,'rockdeng','2014-01-25 06:56:28','ç½‘å¡0 å‡ºåŒ…é‡','ç½‘å¡0 å‡ºåŒ…é‡',0,1,1,0,'2019-08-28 09:07:19',0),(61,52,3,'rockdeng','2014-01-25 06:58:00','sendto å‘é€å¤±è´¥','sendto å‘é€å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(62,51,1,'rockdeng','2014-01-25 07:06:40','æ¨¡å—æ—¥å¿—è®°å½•å‘é€é‡','æ¨¡å—æ—¥å¿—è®°å½•å‘é€é‡',0,1,1,0,'2019-06-06 06:14:36',0),(63,52,3,'rockdeng','2014-01-28 13:18:25','å±žæ€§é“¾è¡¨å«å¼‚å¸¸æ•°æ®','å±žæ€§é“¾è¡¨å«å¼‚å¸¸æ•°æ®',0,1,1,0,'2019-06-06 06:14:36',0),(64,51,1,'rockdeng','2014-01-31 02:23:58','æ”¶åˆ°å¤±è´¥çš„æ—¥å¿—å“åº”åŒ…é‡','æ”¶åˆ°å¤±è´¥çš„æ—¥å¿—å“åº”åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(65,51,1,'rockdeng','2014-01-31 02:24:31','æ”¶åˆ°æ—¥å¿—å“åº”åŒ…é‡','æ”¶åˆ°æ—¥å¿—å“åº”åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(66,52,3,'rockdeng','2014-03-24 13:15:42','å±žæ€§ä¸ŠæŠ¥é“¾è¡¨èŠ‚ç‚¹ç§»é™¤å¤±è´¥','å±žæ€§ä¸ŠæŠ¥é“¾è¡¨èŠ‚ç‚¹ç§»é™¤å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(67,55,1,'rockdeng','2014-03-26 13:36:09','å†™å…¥æ—¥å¿—è®°å½•é‡','å†™å…¥æ—¥å¿—è®°å½•é‡',0,1,1,0,'2019-06-06 06:14:36',0),(68,52,3,'rockdeng','2014-03-27 13:55:34','ç¨‹åºæ ¡éªŒå¼‚å¸¸','ç¨‹åºæ ¡éªŒå¼‚å¸¸',0,1,1,0,'2019-06-06 06:14:36',0),(69,52,3,'rockdeng','2014-03-27 13:56:49','åŠ¨æ€å†…å­˜åˆ†é…å¤±è´¥','åŠ¨æ€å†…å­˜åˆ†é…å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(70,51,1,'rockdeng','2014-03-27 14:06:05','æ—¥å¿—å‘é€åŒ…é‡','æ—¥å¿—å‘é€åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(71,79,1,'rockdeng','2014-03-29 02:33:52','ç½‘å¡0 å…¥æµé‡','ç½‘å¡0 å…¥æµé‡',0,1,1,0,'2019-08-28 09:07:46',0),(72,52,3,'rockdeng','2014-03-29 02:34:43','socket ç»‘å®šå¤±è´¥','socket ç»‘å®šå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(73,53,1,'rockdeng','2014-03-29 02:36:13','pb æ ¼å¼æ—¥å¿—åŒ…é‡','pb æ ¼å¼æ—¥å¿—åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(74,53,3,'rockdeng','2014-03-29 02:36:39','æ”¶åˆ°éžæ³•æ—¥å¿—åŒ…é‡','æ”¶åˆ°éžæ³•æ—¥å¿—åŒ…é‡',0,1,1,1,'2019-06-06 06:14:36',0),(75,53,1,'rockdeng','2014-03-29 02:49:29','æ”¶åˆ°çš„æ—¥å¿—è®°å½•é‡','æ”¶åˆ°çš„æ—¥å¿—è®°å½•é‡',0,1,1,0,'2019-06-06 06:14:36',0),(76,79,1,'rockdeng','2014-03-29 02:59:43','ç½‘å¡0 å‡ºæµé‡','ç½‘å¡0 å‡ºæµé‡',0,1,1,0,'2019-08-28 09:08:12',0),(77,79,1,'rockdeng','2014-03-29 03:04:26','ç½‘å¡1 å…¥åŒ…é‡','ç½‘å¡1 ç½‘å¡åä¸æ˜¯ eth1æ—¶ä½¿ç”¨',0,1,1,0,'2019-08-28 09:09:08',0),(78,52,3,'rockdeng','2014-03-29 03:06:11','è¯»å–æ•°æ®åº“é…ç½®å¤±è´¥','è¯»å–æ•°æ®åº“é…ç½®å¤±è´¥',0,1,1,0,'2019-08-31 11:02:15',0),(79,79,1,'rockdeng','2014-03-29 03:11:14','ç½‘å¡1 å‡ºåŒ…é‡','ç½‘å¡1 å‡ºåŒ…é‡',0,1,1,0,'2019-08-28 09:09:39',0),(80,79,1,'rockdeng','2014-03-29 03:12:25','ç½‘å¡1 å…¥æµé‡','ç½‘å¡1 å…¥æµé‡',0,1,1,0,'2019-08-28 09:10:02',0),(81,52,1,'rockdeng','2014-03-29 03:13:37','æ•°æ®åº“é…ç½®å·²å‘ç”Ÿå˜åŒ–','æ•°æ®åº“é…ç½®å·²å‘ç”Ÿå˜åŒ–',0,1,1,0,'2019-08-31 11:02:03',0),(82,79,1,'rockdeng','2014-03-29 03:14:30','ç½‘å¡1 å‡ºæµé‡','ç½‘å¡1 å‡ºæµé‡',0,1,1,0,'2019-08-28 09:10:39',0),(83,52,3,'rockdeng','2014-03-29 03:15:02','æ•°æ®åº“é…ç½®æ£€æŸ¥æ˜¯å¦æ›´æ–°å¤±è´¥','æ•°æ®åº“é…ç½®æ£€æŸ¥æ˜¯å¦æ›´æ–°å¤±è´¥',0,1,1,0,'2019-08-31 11:01:46',0),(84,55,1,'rockdeng','2014-03-30 13:30:39','è¾“å‡ºæ—¥å¿—æ–‡ä»¶å ç”¨ç£ç›˜ç©ºé—´åˆ°æ–‡ä»¶','è¾“å‡ºæ—¥å¿—æ–‡ä»¶å ç”¨ç£ç›˜ç©ºé—´åˆ°æ–‡ä»¶',0,1,1,0,'2019-06-06 06:14:36',0),(85,52,3,'rockdeng','2017-07-25 00:58:00','å†™æ—¥å¿—èŽ·å–ä¸´ç•Œèµ„æºå¤±è´¥','èŽ·å–ä¸´ç•Œèµ„æºå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(86,52,3,'rockdeng','2017-07-25 01:20:24','å†™æ—¥å¿—ç¼“å†²åŒºæ»¡','å†™æ—¥å¿—ç¼“å†²åŒºæ»¡',0,1,1,1,'2019-06-06 06:14:36',0),(87,52,3,'rockdeng','2017-07-25 01:22:34','vmem å­˜å‚¨å¤±è´¥','vmem å­˜å‚¨å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(88,52,1,'rockdeng','2017-07-25 01:23:41','app  å…±äº«å†…å­˜æŽ¢æµ‹å¤±è´¥','app å…±äº«å†…å­˜æŽ¢æµ‹å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(89,52,3,'rockdeng','2017-07-25 01:29:48','è¯»æ—¥å¿—èŽ·å–ä¸´ç•Œèµ„æºå¤±è´¥','è¯»æ—¥å¿—èŽ·å–ä¸´ç•Œèµ„æºå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(90,52,3,'rockdeng','2017-07-25 15:05:09','å…±äº«å†…å­˜å“ˆå¸Œè¡¨æ ¡éªŒå¤±è´¥','å…±äº«å†…å­˜å“ˆå¸Œè¡¨æ ¡éªŒå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(91,52,3,'rockdeng','2017-07-25 15:06:38','å…±äº«å†…å­˜å“ˆå¸Œè¡¨ä¸´ç•ŒåŒºèµ„æºèŽ·å–å¤±è´¥','å…±äº«å†…å­˜å“ˆå¸Œè¡¨ä¸´ç•ŒåŒºèµ„æºèŽ·å–å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(92,52,3,'rockdeng','2017-07-30 03:20:16','æ£€æŸ¥appä¸»æœåŠ¡å™¨å¤±è´¥','app ä¸»log/attr server check å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(93,51,1,'rockdeng','2017-07-30 08:04:48','slog_client å¿ƒè·³ä¸ŠæŠ¥é‡','æ—¥å¿—å®¢æˆ·ç«¯å¿ƒè·³ä¸ŠæŠ¥',0,1,1,0,'2019-06-06 06:14:36',0),(94,53,1,'rockdeng','2017-07-30 12:32:01','æ”¶åˆ°Logå¿ƒè·³ä¸ŠæŠ¥é‡','æ¥è‡ª slog_client çš„å¿ƒè·³',0,1,1,0,'2019-06-06 06:14:36',0),(97,52,3,'rockdeng','2017-08-12 14:02:11','å…±äº«å†…å­˜åˆ›å»ºå¤±è´¥','å…±äº«å†…å­˜åˆ›å»ºå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(98,52,3,'rockdeng','2017-08-12 14:02:56','å…±äº«å†…å­˜attach æ ¡éªŒå¤±è´¥','å…±äº«å†…å­˜æ ¡éªŒå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(99,52,3,'rockdeng','2017-08-12 15:08:35','comm åº“check å¼‚å¸¸','comm åº“ bug',0,1,1,0,'2019-06-06 06:14:36',0),(100,67,1,'rockdeng','2017-08-13 02:02:34','æ•°æ®å­˜å…¥vmemé‡','æ•°æ®å­˜å…¥vmemé‡',0,1,1,0,'2019-06-06 06:14:36',0),(101,67,3,'rockdeng','2017-08-13 02:06:11','æ•°æ®å­˜å…¥vmemå¤±è´¥é‡','æ•°æ®å­˜å…¥vmemå¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36',0),(102,67,1,'rockdeng','2017-08-13 02:07:56','ä»Žvmem å–å‡ºæ•°æ®æˆåŠŸé‡','ä»Žvmem å–å‡ºæ•°æ®æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36',0),(103,67,3,'rockdeng','2017-08-13 02:08:51','ä»Žvmem å–å‡ºæ•°æ®å¼‚å¸¸','ä»Žvmem å–å‡ºæ•°æ®å¼‚å¸¸',0,1,1,0,'2019-06-06 06:14:36',0),(104,67,3,'rockdeng','2017-08-13 02:11:33','vmem é‡Šæ”¾å¼‚å¸¸','vmem é‡Šæ”¾å¼‚å¸¸',0,1,1,0,'2019-06-06 06:14:36',0),(105,67,1,'rockdeng','2017-08-13 02:13:06','vmem å»¶è¿Ÿé‡Šæ”¾é‡','vmem å»¶è¿Ÿé‡Šæ”¾é‡',0,1,1,0,'2019-06-06 06:14:36',0),(106,67,1,'rockdeng','2017-08-13 02:14:09','vmem å»¶è¿Ÿé‡Šæ”¾æˆåŠŸ','vmem å»¶è¿Ÿé‡Šæ”¾',0,1,1,0,'2019-06-06 06:14:36',0),(107,52,1,'rockdeng','2017-08-13 09:02:29','free','å½“å‰æœ‰æ—¥å¿—äº§ç”Ÿçš„app æ•°ç›®',0,1,1,0,'2019-08-28 09:57:30',0),(108,69,3,'rockdeng','2017-08-15 02:07:41','cgi å¯åŠ¨å¤±è´¥','cgi æ‰§è¡Œå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(109,52,3,'rockdeng','2017-08-15 02:11:50','login å¤±è´¥',' å¤±è´¥',0,1,1,0,'2019-09-03 09:45:15',0),(110,69,1,'rockdeng','2017-08-15 02:24:55','login cookie check å¤±è´¥','login cookie check å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(111,70,1,'rockdeng','2017-08-15 02:26:09','login cookie check æˆåŠŸé‡','login cookie check æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36',0),(112,70,1,'rockdeng','2017-08-15 02:28:11','login å¼¹å‡ºç™»å½•æ¡†','login å¼¹å‡ºç™»å½•æ¡†',0,1,1,0,'2019-06-06 06:14:36',0),(113,70,1,'rockdeng','2017-08-15 02:28:29','login é€šè¿‡è¾“å…¥ç”¨æˆ·åå¯†ç ç™»å½•æˆåŠŸ','login é€šè¿‡è¾“å…¥ç”¨æˆ·åå¯†ç ç™»å½•æˆåŠŸ',0,1,1,0,'2019-06-06 06:14:36',0),(114,52,1,'rockdeng','2017-08-15 02:31:19','éªŒè¯ç éªŒè¯æˆåŠŸé‡','éªŒè¯ç éªŒè¯æˆåŠŸé‡',0,1,1,0,'2019-09-03 09:44:01',0),(115,52,1,'rockdeng','2017-08-15 02:34:46','éªŒè¯ç éªŒè¯å¤±è´¥é‡','éªŒè¯ç éªŒè¯å¤±è´¥é‡',0,1,1,0,'2019-09-03 09:43:54',0),(116,71,3,'rockdeng','2017-08-15 05:35:37','mysql è¿žæŽ¥å¤±è´¥','mysql è¿žæŽ¥å¤±è´¥',0,1,1,1,'2019-06-06 06:14:36',0),(117,71,1,'rockdeng','2017-08-15 06:22:24','mysql è¿žæŽ¥è€—æ—¶0-10ms','mysql è¿žæŽ¥è€—æ—¶0-10ms',0,1,1,0,'2019-06-06 06:14:36',0),(118,71,1,'rockdeng','2017-08-15 06:22:39','mysql è¿žæŽ¥è€—æ—¶10-20ms','mysql è¿žæŽ¥è€—æ—¶10-20ms',0,1,1,0,'2019-06-06 06:14:36',0),(119,71,1,'rockdeng','2017-08-15 06:22:50','mysql è¿žæŽ¥è€—æ—¶20-50ms','mysql è¿žæŽ¥è€—æ—¶20-50ms',0,1,1,0,'2019-06-06 06:14:36',0),(120,71,1,'rockdeng','2017-08-15 06:23:03','mysql è¿žæŽ¥è€—æ—¶50-100ms','mysql è¿žæŽ¥è€—æ—¶50-100ms',0,1,1,0,'2019-06-06 06:14:36',0),(121,71,1,'rockdeng','2017-08-15 06:23:17','mysql è¿žæŽ¥è€—æ—¶å¤§äºŽ1000ms','mysql è¿žæŽ¥è€—æ—¶å¤§äºŽ1000ms',0,1,1,0,'2019-06-06 06:14:36',0),(122,71,1,'rockdeng','2017-08-15 09:15:37','mysql æ‰§è¡Œæ—¶é—´ 0-20 ms','mysql æ‰§è¡Œæ—¶é—´ 0-20 ms',0,1,1,0,'2019-06-06 06:14:36',0),(123,71,1,'rockdeng','2017-08-15 09:15:47','mysql æ‰§è¡Œæ—¶é—´ 20-50 ms','mysql æ‰§è¡Œæ—¶é—´ 20-50 ms',0,1,1,0,'2019-06-06 06:14:36',0),(124,71,1,'rockdeng','2017-08-15 09:15:57','mysql æ‰§è¡Œæ—¶é—´ 50-100 ms','mysql æ‰§è¡Œæ—¶é—´ 50-100 ms',0,1,1,0,'2019-06-06 06:14:36',0),(125,71,1,'rockdeng','2017-08-15 09:16:10','mysql æ‰§è¡Œæ—¶é—´ 100-200 ms','mysql æ‰§è¡Œæ—¶é—´ 100-200 ms',0,1,1,0,'2019-06-06 06:14:36',0),(126,71,1,'rockdeng','2017-08-15 09:16:24','mysql æ‰§è¡Œæ—¶é—´ 200-500 ms','mysql æ‰§è¡Œæ—¶é—´ 200-500 ms',0,1,1,0,'2019-06-06 06:14:36',0),(127,71,1,'rockdeng','2017-08-15 09:16:39','mysql æ‰§è¡Œæ—¶é—´ 500-1000 ms','mysql æ‰§è¡Œæ—¶é—´ 500-1000 ms',0,1,1,0,'2019-06-06 06:14:36',0),(128,71,1,'rockdeng','2017-08-15 09:16:58','mysql æ‰§è¡Œæ—¶é—´ 1000-2000 ms','mysql æ‰§è¡Œæ—¶é—´ 1000-2000 ms',0,1,1,0,'2019-06-06 06:14:36',0),(129,71,1,'rockdeng','2017-08-15 09:17:19','mysql æ‰§è¡Œæ—¶é—´å¤§äºŽ 2000','mysql æ‰§è¡Œæ—¶é—´å¤§äºŽ 2000 ms',0,1,1,0,'2019-06-06 06:14:36',0),(130,71,1,'rockdeng','2017-08-15 11:40:26','mysql è¿žæŽ¥è€—æ—¶åœ¨ 100-200 ms','mysql è¿žæŽ¥è€—æ—¶åœ¨ 100-200 ms',0,1,1,0,'2019-06-06 06:14:36',0),(131,71,1,'rockdeng','2017-08-15 11:42:09','mysql è¿žæŽ¥è€—æ—¶åœ¨ 200-500 ms','mysql è¿žæŽ¥è€—æ—¶åœ¨ 200-500 ms',0,1,1,0,'2019-06-06 06:14:36',0),(132,71,1,'rockdeng','2017-08-15 11:42:25','mysql è¿žæŽ¥è€—æ—¶åœ¨ 500-1000 ms','mysql è¿žæŽ¥è€—æ—¶åœ¨ 500-1000 ms',0,1,1,0,'2019-06-06 06:14:36',0),(133,71,1,'rockdeng','2017-08-15 11:48:13','mysql get result å®Œæˆæ—¶é—´ 0-20ms','mysql get result å®Œæˆæ—¶é—´ 0-20ms',0,1,1,0,'2019-06-06 06:14:36',0),(134,71,1,'rockdeng','2017-08-15 11:48:25','mysql get result å®Œæˆæ—¶é—´ 20-50ms','mysql get result å®Œæˆæ—¶é—´ 20-50ms',0,1,1,0,'2019-06-06 06:14:36',0),(135,71,1,'rockdeng','2017-08-15 11:48:40','mysql get result å®Œæˆæ—¶é—´ 50-100ms','mysql get result å®Œæˆæ—¶é—´ 50-100ms',0,1,1,0,'2019-06-06 06:14:36',0),(136,71,1,'rockdeng','2017-08-15 11:48:56','mysql get result å®Œæˆæ—¶é—´ 100-200ms','mysql get result å®Œæˆæ—¶é—´ 100-200ms',0,1,1,0,'2019-06-06 06:14:36',0),(137,71,1,'rockdeng','2017-08-15 11:49:09','mysql get result å®Œæˆæ—¶é—´ 200-500ms','mysql get result å®Œæˆæ—¶é—´ 200-500ms',0,1,1,0,'2019-06-06 06:14:36',0),(138,71,1,'rockdeng','2017-08-15 11:49:25','mysql get result å®Œæˆæ—¶é—´ 500-1000ms','mysql get result å®Œæˆæ—¶é—´ 500-1000ms',0,1,1,0,'2019-06-06 06:14:36',0),(139,71,1,'rockdeng','2017-08-15 11:49:50','mysql get result å®Œæˆæ—¶é—´å¤§äºŽ1000ms','mysql get result å®Œæˆæ—¶é—´å¤§äºŽ1000ms',0,1,1,0,'2019-06-06 06:14:36',0),(142,69,1,'rockdeng','2017-08-19 13:09:51','cgi è¯·æ±‚å¤±è´¥é‡','cgi è¯·æ±‚å¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36',0),(143,70,1,'rockdeng','2017-08-19 13:10:02','cgi è¯·æ±‚æˆåŠŸé‡','cgi è¯·æ±‚æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36',0),(144,72,1,'rockdeng','2017-08-19 13:19:11','cgi å“åº”è€—æ—¶åœ¨ 0-100ms ','cgi å“åº”è€—æ—¶åœ¨ 0-100ms ',0,1,15,0,'2019-06-06 06:14:36',0),(145,72,1,'testmanager','2017-08-26 06:18:42','cgi å“åº”è€—æ—¶åœ¨ 100-300 ms','cgi å“åº”è€—æ—¶åœ¨ 100-300 ms',0,15,15,0,'2019-06-06 06:14:36',0),(146,72,1,'testmanager','2017-08-26 11:54:32','cgi å“åº”è€—æ—¶åœ¨ 300-500 ms','cgi å“åº”è€—æ—¶åœ¨ 300-500 ms',0,15,15,0,'2019-06-06 06:14:36',0),(147,72,1,'testmanager','2017-08-26 11:54:54','cgi å“åº”è€—æ—¶å¤§äºŽç­‰äºŽ 500 ms','cgi å“åº”è€—æ—¶å¤§äºŽç­‰äºŽ 500 ms',0,15,15,0,'2019-06-06 06:14:36',0),(163,76,4,'rockdeng','2017-10-21 10:25:39','cpu ä½¿ç”¨çŽ‡','cpu æ•´åˆä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(164,76,4,'rockdeng','2017-10-21 10:38:33','cpu0 ä½¿ç”¨çŽ‡','cpu0 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(165,76,4,'rockdeng','2017-10-21 10:42:17','cpu1 ä½¿ç”¨çŽ‡','cpu1 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(166,76,4,'rockdeng','2017-10-21 11:28:40','cpu2 ä½¿ç”¨çŽ‡','cpu2 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(167,76,4,'rockdeng','2017-10-21 11:28:50','cpu3 ä½¿ç”¨çŽ‡','cpu3 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(168,76,4,'rockdeng','2017-10-21 11:28:59','cpu4 ä½¿ç”¨çŽ‡','cpu4 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(169,76,4,'rockdeng','2017-10-21 11:30:05','cpu5 ä½¿ç”¨çŽ‡','cpu5 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(170,76,4,'rockdeng','2017-10-21 11:33:02','cpu6 ä½¿ç”¨çŽ‡','cpu6 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(171,76,4,'rockdeng','2017-10-21 11:33:11','cpu7 ä½¿ç”¨çŽ‡','cpu7 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(172,76,4,'rockdeng','2017-10-21 11:33:18','cpu8 ä½¿ç”¨çŽ‡','cpu8 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(173,76,4,'rockdeng','2017-10-21 11:33:25','cpu9 ä½¿ç”¨çŽ‡','cpu9 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(174,76,4,'rockdeng','2017-10-21 11:33:39','cpu10 ä½¿ç”¨çŽ‡','cpu10 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(175,76,4,'rockdeng','2017-10-21 11:33:47','cpu11 ä½¿ç”¨çŽ‡','cpu11 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(176,76,4,'rockdeng','2017-10-21 11:33:54','cpu12 ä½¿ç”¨çŽ‡','cpu12 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(177,76,4,'rockdeng','2017-10-21 11:33:59','cpu13 ä½¿ç”¨çŽ‡','cpu13 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(178,76,4,'rockdeng','2017-10-21 11:34:06','cpu14 ä½¿ç”¨çŽ‡','cpu14 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(179,76,4,'rockdeng','2017-10-21 11:34:11','cpu15 ä½¿ç”¨çŽ‡','cpu15 ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(180,52,3,'rockdeng','2017-10-21 12:12:50','å“ˆå¸Œé˜¶æ•°ä½¿ç”¨è¾¾åˆ°é¢„è­¦å€¼','å“ˆå¸Œé˜¶æ•°ä½¿ç”¨è¾¾åˆ°é¢„è­¦å€¼',0,1,1,0,'2019-06-06 06:14:36',0),(181,52,3,'rockdeng','2017-10-21 12:20:57','å“ˆå¸Œé˜¶æ•°ä½¿ç”¨è€—å…‰','å“ˆå¸Œé˜¶æ•°ä½¿ç”¨è€—å…‰',0,1,1,0,'2019-06-06 06:14:36',0),(182,77,1,'rockdeng','2017-10-22 06:21:52','ç£ç›˜åˆ†åŒº1ä½¿ç”¨çŽ‡è¶…è¿‡é¢„è­¦å€¼','ç£ç›˜åˆ†åŒº1ä½¿ç”¨çŽ‡è¶…è¿‡é¢„è­¦å€¼',0,1,1,0,'2019-06-06 06:14:36',0),(183,78,4,'rockdeng','2017-10-29 03:50:27','å†…å­˜ä½¿ç”¨çŽ‡','å†…å­˜ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(184,77,4,'rockdeng','2017-10-29 09:17:54','æ€»ç£ç›˜ç©ºé—´ä½¿ç”¨çŽ‡','æ‰€æœ‰æŒ‚è½½çš„ç£ç›˜ç©ºé—´æ±‡æ€»åŽçš„ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(185,77,3,'rockdeng','2017-10-29 09:18:32','ç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡80%','æŸç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡80%',0,1,1,0,'2019-06-06 06:14:36',0),(186,77,3,'rockdeng','2017-10-29 10:08:49','ç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡70%','æŸç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡70%',0,1,1,0,'2019-06-06 06:14:36',0),(187,77,3,'rockdeng','2017-10-29 10:08:59','ç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡90%','æŸç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡90%',0,1,1,0,'2019-06-06 06:14:36',0),(188,77,3,'rockdeng','2017-10-29 10:09:07','ç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡95%','æŸç£ç›˜åˆ†åŒºä½¿ç”¨çŽ‡è¶…è¿‡95%',0,1,1,0,'2019-06-06 06:14:36',0),(189,77,4,'rockdeng','2017-10-29 12:00:15','ç£ç›˜åˆ†åŒºæœ€å¤§ä½¿ç”¨çŽ‡','ç£ç›˜åˆ†åŒºæœ€å¤§ä½¿ç”¨çŽ‡',0,1,1,0,'2019-06-06 06:14:36',0),(190,79,1,'rockdeng','2017-11-05 06:45:36','ç½‘å¡eth0 å‡ºæµé‡','ç½‘å¡eth0 å‡ºæµé‡',0,1,1,0,'2019-06-06 06:14:36',0),(191,79,1,'rockdeng','2017-11-05 06:45:47','ç½‘å¡eth0 å…¥æµé‡','ç½‘å¡eth0 å…¥æµé‡',0,1,1,0,'2019-06-06 06:14:36',0),(192,79,1,'rockdeng','2017-11-05 06:46:30','ç½‘å¡eth0 å‡ºåŒ…é‡','ç½‘å¡eth0 ä»ŽåŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(193,79,1,'rockdeng','2017-11-05 06:46:49','ç½‘å¡eth0 å…¥åŒ…é‡','ç½‘å¡eth0 å…¥åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(194,79,1,'rockdeng','2017-11-05 06:47:33','ç½‘å¡eth1 å‡ºæµé‡','ç½‘å¡eth1 å‡ºæµé‡',0,1,1,0,'2019-06-06 06:14:36',0),(195,79,1,'rockdeng','2017-11-05 06:47:56','ç½‘å¡eth1 å…¥æµé‡','ç½‘å¡eth1 å…¥æµé‡',0,1,1,0,'2019-06-06 06:14:36',0),(196,79,1,'rockdeng','2017-11-05 06:48:30','ç½‘å¡eth1 å‡ºåŒ…é‡','ç½‘å¡eth1 å‡ºåŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(197,79,1,'rockdeng','2017-11-05 06:50:01','ç½‘å¡eth1 å…¥åŒ…é‡','ç½‘å¡eth1 å…¥åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(198,79,1,'rockdeng','2017-11-05 06:51:31','ç½‘å¡lo å‡ºæµé‡','ç½‘å¡lo å‡ºæµé‡',0,1,1,0,'2019-06-06 06:14:36',0),(199,79,1,'rockdeng','2017-11-05 06:51:44','ç½‘å¡lo å…¥æµé‡','ç½‘å¡lo å…¥æµé‡',0,1,1,0,'2019-06-06 06:14:36',0),(200,79,1,'rockdeng','2017-11-05 06:52:00','ç½‘å¡lo å‡ºåŒ…é‡','ç½‘å¡lo å‡ºåŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(201,79,1,'rockdeng','2017-11-05 06:52:17','ç½‘å¡lo å…¥åŒ…é‡','ç½‘å¡lo å…¥åŒ…é‡',0,1,1,0,'2019-06-06 06:14:36',0),(202,79,1,'rockdeng','2017-11-05 12:47:57','ç½‘å¡å‡ºçŽ°ä¸¢åŒ…','ç½‘å¡å‡ºçŽ°ä¸¢åŒ…',0,1,1,0,'2019-08-28 09:12:49',0),(203,80,1,'rockdeng','2017-12-05 12:01:29','memcache get æˆåŠŸé‡','memcache get æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36',0),(204,80,1,'rockdeng','2017-12-05 12:01:43','memcache get æ€»é‡','memcache get æ€»é‡',0,1,1,0,'2019-06-06 06:14:36',0),(205,80,1,'rockdeng','2017-12-05 12:01:53','memcache get å¤±è´¥é‡','memcache get å¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36',0),(206,80,1,'rockdeng','2017-12-05 12:19:44','memcache set å¤±è´¥é‡','memcache set å¤±è´¥é‡',0,1,1,0,'2019-06-06 06:14:36',0),(207,80,1,'rockdeng','2017-12-05 12:19:58','memcache set æˆåŠŸé‡','memcache set æˆåŠŸé‡',0,1,1,0,'2019-06-06 06:14:36',0),(208,80,1,'rockdeng','2017-12-05 12:20:09','memcache set æ€»é‡','memcache set æ€»é‡',0,1,1,0,'2019-06-06 06:14:36',0),(219,52,3,'rockdeng','2018-10-10 11:59:39','slog_cgid å¿ƒè·³æ£€æµ‹å¤±è´¥','slog_cgid å¿ƒè·³æ£€æµ‹å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(220,53,1,'rockdeng','2018-10-14 03:20:38','client logä¸ŠæŠ¥æ—¶é—´è§¦å‘æ ¡å‡†','client ä¸Ž server çš„æ—¶é—´ç›¸å·®è¶…è¿‡2åˆ†é’Ÿ',0,1,1,0,'2019-06-06 06:14:36',0),(222,52,3,'rockdeng','2018-10-20 04:20:42','é‚®ä»¶å…±äº«å†…å­˜ä¸´ç•Œèµ„æºèŽ·å–å¤±è´¥','å…±äº«å†…å­˜ä¸´ç•Œèµ„æº',0,1,1,0,'2019-06-06 06:14:36',0),(223,82,1,'rockdeng','2018-10-20 04:27:32','é‚®ä»¶è¿‡æœŸæœªå‘é€é‡','é‚®ä»¶è¿‡æœŸæœªå‘é€é‡',0,1,1,0,'2019-06-06 06:14:36',0),(224,82,1,'rockdeng','2018-10-20 08:23:41','é‚®ä»¶å‘é€é‡','æ€»é‚®ä»¶å‘é€é‡',0,1,1,0,'2019-06-06 06:14:36',0),(225,82,1,'rockdeng','2018-10-20 08:30:59','é‚®ä»¶å‘é€å¤±è´¥é‡','æ€»é‚®ä»¶å‘é€å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(226,52,3,'rockdeng','2018-10-28 08:19:09','user pb session è®¾ç½®å¤±è´¥','UserSessionInfo ç»“æž„å­˜å‚¨',0,1,1,0,'2019-06-06 06:14:36',0),(227,52,3,'rockdeng','2018-10-28 08:19:25','user pb session èŽ·å–å¤±è´¥','UserSessionInfo ç»“æž„èŽ·å–',0,1,1,0,'2019-06-06 06:14:36',0),(234,55,3,'rockdeng','2018-11-13 08:55:24','ä»Žvmem èŽ·å–æ—¥å¿—å†…å®¹å‡ºé”™','ä»Žvmem èŽ·å–æ—¥å¿—å†…å®¹å‡ºé”™',0,1,1,0,'2019-06-06 06:14:36',0),(235,55,3,'rockdeng','2018-11-13 08:55:47','æ ¡éªŒvmem æ—¥å¿—å‡ºé”™','æ ¡éªŒvmem æ—¥å¿—å‡ºé”™',0,1,1,0,'2019-06-06 06:14:36',0),(236,52,3,'rockdeng','2018-11-13 12:23:51','ç›‘æŽ§ç‚¹ä¸ŠæŠ¥å¤±è´¥','ç›‘æŽ§ç‚¹ä¸ŠæŠ¥å¤±è´¥',0,1,1,1,'2019-06-06 06:14:36',0),(239,52,3,'rockdeng','2018-11-14 03:18:35','å“ˆå¸Œè¡¨æ•°æ®éžæ³•å·²é‡ç½®','å“ˆå¸Œè¡¨æ•°æ®éžæ³•å·²é‡ç½®',0,1,1,0,'2019-06-06 06:14:36',0),(240,52,3,'rockdeng','2018-11-14 11:40:22','åº”ç”¨æ—¥å¿—ç¼“å­˜å…±äº«å†…å­˜åˆ›å»ºå¤±è´¥','åº”ç”¨æ—¥å¿—ç¼“å­˜å…±äº«å†…å­˜åˆ›å»ºå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(241,52,3,'rockdeng','2018-11-14 11:45:17','åº”ç”¨åŽ†å²æ—¥å¿—æ–‡ä»¶æ ¡éªŒå¤±è´¥','åº”ç”¨åŽ†å²æ—¥å¿—æ–‡ä»¶æ ¡éªŒå¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(248,52,3,'rockdeng','2018-11-17 11:42:53','ç”¨æˆ·ä¸»è´¦å·ä¿¡æ¯pb åºåˆ—åŒ–å¤±è´¥','ç”¨æˆ·ä¸»è´¦å·ä¿¡æ¯pb åºåˆ—åŒ–å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(249,52,1,'rockdeng','2018-11-17 11:47:39','ç”¨æˆ·ä¸»è´¦å·ä¿¡æ¯pb åºåˆ—åŒ–ç¼“å†²åŒºå³å°†è€—å°½','ç”¨æˆ·ä¸»è´¦å·ä¿¡æ¯pb åºåˆ—åŒ–ç¼“å†²åŒºå³å°†è€—å°½',0,1,1,0,'2019-06-06 06:14:36',0),(250,52,3,'rockdeng','2018-11-17 11:49:25','ç”¨æˆ·ç™»å½•sessionåºåˆ—åŒ–ç¼“å†²åŒºå³å°†è€—å°½','ç”¨æˆ·ç™»å½•sessionåºåˆ—åŒ–ç¼“å†²åŒºå³å°†è€—å°½',0,1,1,0,'2019-06-06 06:14:36',0),(251,52,3,'rockdeng','2018-11-17 11:52:31','ç”¨æˆ·ä¸»è´¦å·ä¿¡æ¯ååºåˆ—åŒ–å¤±è´¥','ç”¨æˆ·ä¸»è´¦å·ä¿¡æ¯ååºåˆ—åŒ–å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(252,52,3,'rockdeng','2018-11-17 13:15:15','ç³»ç»Ÿå¼‚å¸¸é”™è¯¯','ç³»ç»Ÿå¼‚å¸¸é”™è¯¯',0,1,1,1,'2019-06-06 06:14:36',0),(253,52,1,'rockdeng','2018-11-17 14:38:59','ç”¨æˆ·æ—¥å¿—è¾¾åˆ°ç©ºé—´ä¸Šé™è§¦å‘æ¬¡æ•°','ç”¨æˆ·æ—¥å¿—è¾¾åˆ°ç©ºé—´ä¸Šé™è§¦å‘æ¬¡æ•°',0,1,1,0,'2019-08-31 10:59:25',0),(254,53,1,'rockdeng','2018-11-18 07:50:15','åˆ é™¤app æ—¥å¿—æ–‡ä»¶è¯·æ±‚é‡','åˆ é™¤app æ—¥å¿—æ–‡ä»¶è¯·æ±‚é‡',0,1,1,0,'2019-06-06 06:14:36',0),(255,53,1,'rockdeng','2018-11-18 08:19:54','æŸ¥è¯¢ app æ—¥å¿—æ–‡ä»¶å ç”¨ç©ºé—´è¯·æ±‚é‡','æŸ¥è¯¢ app æ—¥å¿—æ–‡ä»¶å ç”¨ç©ºé—´è¯·æ±‚é‡',0,1,1,0,'2019-06-06 06:14:36',0),(256,53,1,'rockdeng','2018-11-18 09:17:12','ååºåˆ—åŒ–è¯·æ±‚åŒ…å¤±è´¥','ååºåˆ—åŒ–è¯·æ±‚åŒ…å¤±è´¥',0,1,1,0,'2019-06-06 06:14:36',0),(257,53,1,'rockdeng','2018-11-18 09:19:11','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´è¯·æ±‚é‡','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´è¯·æ±‚é‡',0,1,1,0,'2019-06-06 06:14:36',0),(258,53,1,'rockdeng','2018-11-18 09:52:54','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡','æ”¶åˆ°æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡',0,1,1,0,'2019-06-06 06:14:36',0),(259,53,1,'rockdeng','2018-11-18 09:53:22','å‘é€æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡','å‘é€æŸ¥è¯¢ app æ—¥å¿—å ç”¨ç©ºé—´å“åº”é‡',0,1,1,0,'2019-06-06 06:14:36',0),(260,52,3,'rockdeng','2018-11-18 11:42:59','app æ—¥å¿—åˆ†å‘æœåŠ¡å™¨é”™è¯¯','app æ—¥å¿—åˆ†å‘æœåŠ¡å™¨é”™è¯¯',0,1,1,1,'2019-06-06 06:14:36',0),(261,52,3,'rockdeng','2018-11-18 12:12:06','ç›‘æŽ§ç‚¹æœåŠ¡å™¨åˆ†å‘é”™è¯¯','ç›‘æŽ§ç‚¹æœåŠ¡å™¨åˆ†å‘é”™è¯¯',0,1,1,0,'2019-06-06 06:14:36',0),(262,53,1,'rockdeng','2018-11-19 07:22:34','è®¾ç½®æ—¥å¿—æ–‡ä»¶åˆ é™¤æ ‡è®°é‡','è®¾ç½®æ—¥å¿—æ–‡ä»¶åˆ é™¤æ ‡è®°é‡',0,1,1,0,'2019-06-06 06:14:36',0),(263,55,1,'rockdeng','2018-11-19 08:05:18','é€šè¿‡åˆ é™¤æ ‡è®°åˆ é™¤æ—¥å¿—æ–‡ä»¶é‡','é€šè¿‡åˆ é™¤æ ‡è®°åˆ é™¤æ—¥å¿—æ–‡ä»¶é‡',0,1,1,0,'2019-06-06 06:14:36',0),(284,52,1,'rockdeng','2018-12-06 13:39:33','äºŒè¿›åˆ¶æ ¼å¼æ—¥å¿—åŒ…é‡','äºŒè¿›åˆ¶æ ¼å¼æ—¥å¿—åŒ…é‡',0,1,1,0,'2019-08-31 12:16:48',0),(285,83,1,'rockdeng','2018-12-06 13:51:29','å½“å‰å‘Šè­¦äº§ç”Ÿé‡','å½“å‰å‘Šè­¦äº§ç”Ÿé‡',0,1,1,0,'2019-08-28 08:50:18',0),(330,53,5,'rockdeng','2019-05-21 03:37:13','è¿œç¨‹æ€»æ—¥å¿—è®°å½•é‡','agent client å‘é€è¿‡æ¥çš„æ—¥å¿—é‡',0,1,1,0,'2019-06-06 06:14:36',0),(331,53,5,'rockdeng','2019-05-21 03:37:27','ç›‘æŽ§ç³»ç»Ÿæœ¬èº«çš„æ—¥å¿—è®°å½•é‡','ç›‘æŽ§ç³»ç»Ÿæœ¬èº«çš„æ—¥å¿—è®°å½•é‡',0,1,1,0,'2019-08-31 11:26:58',0),(332,55,5,'rockdeng','2019-05-21 03:51:54','æ—¥å¿—è®°å½•å†™å…¥ç£ç›˜æ€»é‡','æ—¥å¿—æ€»é‡',0,1,1,0,'2019-06-06 06:14:36',0),(355,82,1,'sadmin','2019-07-03 08:27:27','é‚®ä»¶å‘é€é‡','æ¯åˆ†é’Ÿçš„é‚®ä»¶å‘é€é‡',0,1,1,0,'2019-07-03 08:27:27',0),(356,82,5,'sadmin','2019-07-03 08:27:53','é‚®ä»¶å‘é€é‡åŽ†å²æ€»é‡','æ€»çš„é‚®ä»¶å‘é€é‡',0,1,1,0,'2019-07-03 08:27:53',0);
/*!40000 ALTER TABLE `mt_attr` ENABLE KEYS */;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_ins after insert on mt_attr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr', new.id); end */;;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_up after update on mt_attr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr', old.id); end */;;
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
  `type` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `parent_type` int(11) unsigned DEFAULT '0',
  `type_pos` varchar(128) DEFAULT '0' COMMENT 'ç±»åž‹ä½ç½®',
  `name` varchar(64) NOT NULL COMMENT 'ç±»åž‹åç§°',
  `attr_desc` varchar(256) DEFAULT NULL COMMENT 'ç±»åž‹æè¿°',
  `status` tinyint(4) DEFAULT '0' COMMENT 'çŠ¶æ€ç  0-æ­£å¸¸ä½¿ç”¨ 1-åˆ é™¤æ ‡å¿—',
  `create_user` varchar(64) DEFAULT NULL,
  `mod_user` varchar(64) DEFAULT NULL,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `user_add_id` int(11) unsigned DEFAULT '0',
  `user_mod_id` int(11) unsigned DEFAULT '0',
  PRIMARY KEY (`type`)
) ENGINE=InnoDB AUTO_INCREMENT=84 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_attr_type`
--

LOCK TABLES `mt_attr_type` WRITE;
/*!40000 ALTER TABLE `mt_attr_type` DISABLE KEYS */;
INSERT INTO `mt_attr_type` VALUES (1,0,'1','ç›‘æŽ§ç‚¹æ ¹ç±»åž‹','ç›‘æŽ§ç‚¹æ ¹ç±»åž‹',0,'sadmin','sadmin','2019-07-02 06:39:16','0000-00-00 00:00:00',1,1),(50,1,'1.1','æ—¥å¿—ç³»ç»Ÿ','æ—¥å¿—ç³»ç»Ÿå„æ¨¡å—ç›‘æŽ§',0,'rockdeng','rockdeng','2019-07-02 06:55:46','0000-00-00 00:00:00',1,1),(51,50,'1.1.1','slog_client ç›‘æŽ§','æ—¥å¿—ç³»ç»Ÿå®¢æˆ·ç«¯ç›¸å…³ç›‘æŽ§ä¸ŠæŠ¥',0,'rockdeng','rockdeng','2019-07-02 06:55:53','0000-00-00 00:00:00',1,1),(52,1,'1.1','åºŸå¼ƒå¾…ç”¨çš„ç›‘æŽ§ç‚¹','åºŸå¼ƒç­‰å¾…ä¿®æ”¹ä½¿ç”¨çš„ç›‘æŽ§ç‚¹',0,'rockdeng','sadmin','2019-08-28 09:57:05','0000-00-00 00:00:00',1,1),(53,50,'1.1.1','slog_serve','slog æ—¥å¿—æœåŠ¡å™¨ç«¯',0,'rockdeng','rockdeng','2019-07-02 06:55:53','0000-00-00 00:00:00',1,1),(54,50,'1.1.1','slog_config','è¯»å– mysql é…ç½®çš„è¿›ç¨‹',0,'rockdeng','rockdeng','2019-07-02 06:55:53','0000-00-00 00:00:00',1,1),(55,50,'1.1.1','slog_write','æ—¥å¿—æœåŠ¡å™¨ç«¯å†™æ—¥å¿—è¿›ç¨‹çš„ç›¸å…³ä¸ŠæŠ¥',0,'sadmin','sadmin','2019-07-02 06:39:16','0000-00-00 00:00:00',1,1),(56,1,'1.1','ç›‘æŽ§ç³»ç»Ÿ','å±žæ€§ä¸ŠæŠ¥ç›‘æŽ§ç³»ç»Ÿ',0,'sadmin','sadmin','2019-07-02 06:39:16','0000-00-00 00:00:00',1,1),(57,56,'1.1.1','monitor_client','ç›‘æŽ§ç³»ç»Ÿå®¢æˆ·ç«¯ monitor_client',0,'sadmin','sadmin','2019-07-02 06:39:16','0000-00-00 00:00:00',1,1),(58,56,'1.1.1','monitor_server','ç›‘æŽ§ç³»ç»ŸæœåŠ¡å™¨ç«¯ slog_monitor_server',0,'sadmin','sadmin','2019-07-02 06:39:16','0000-00-00 00:00:00',1,1),(66,1,'1.1','é€šç”¨ç»„ä»¶ä¸ŠæŠ¥','é€šç”¨ç»„ä»¶ç›‘æŽ§ä¸ŠæŠ¥',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-08-13 02:01:57',1,1),(67,66,'1.1.1','vmem','vmem å¯å˜é•¿å…±äº«å†…å­˜ç»„ä»¶',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-08-13 02:04:48',1,1),(68,1,'1.1','cgi ç›‘æŽ§','ç›‘æŽ§ç‚¹ç³»ç»Ÿ cgi/fcgi ç›¸å…³çš„ç›‘æŽ§ä¸ŠæŠ¥',0,'sadmin','sadmin','2019-07-02 06:40:00','2017-08-15 02:05:47',1,1),(69,68,'1.1.1','cgi å¼‚å¸¸ç›‘æŽ§','cgi å¼‚å¸¸ç‚¹ä¸ŠæŠ¥s',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-08-15 02:06:23',1,15),(70,68,'1.1.1','cgi ä¸šåŠ¡é‡ç›‘æŽ§','cgi ä¸šåŠ¡é‡ç›‘æŽ§',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-08-15 02:06:46',1,1),(71,66,'1.1.1','mysqlwrappe','mysql ç»„ä»¶',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-08-15 05:35:06',1,1),(72,68,'1.1.1','cgi è°ƒç”¨è€—æ—¶ç›‘æŽ§','ç›‘æŽ§ç³»ç»Ÿ cgi è°ƒç”¨è€—æ—¶ç›‘æŽ§',0,'sadmin','sadmin','2019-07-02 06:39:48','2017-08-19 13:21:20',1,1),(75,1,'1.1','æœåŠ¡å™¨åŸºç¡€ç›‘æŽ§','ç›‘æŽ§ç‚¹ç±»åž‹ï¼Œæ‰€æœ‰ç”¨æˆ·å¯ä¸ŠæŠ¥',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-10-21 10:24:44',1,1),(76,75,'1.1.1','cpu ç›‘æŽ§','cpuä½¿ç”¨çŽ‡ç­‰ç›‘æŽ§',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-10-21 10:36:58',1,1),(77,75,'1.1.1','ç£ç›˜ç›‘æŽ§','ç£ç›˜ä½¿ç”¨çŽ‡ç­‰ç›‘æŽ§',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-10-22 06:18:25',1,1),(78,75,'1.1.1','å†…å­˜ç›‘æŽ§','å†…å­˜ç›¸å…³çš„é€šç”¨ç›‘æŽ§',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-10-29 03:49:59',1,1),(79,75,'1.1.1','ç½‘ç»œç›‘æŽ§','ç½‘ç»œç›¸å…³çš„ç›‘æŽ§',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-11-05 06:43:25',1,1),(80,66,'1.1.1','memcache','memcache ç›‘æŽ§',0,'sadmin','sadmin','2019-07-02 06:39:16','2017-12-05 12:00:48',1,1),(82,66,'1.1.1','mail','é‚®ä»¶å‘é€ç›‘æŽ§',0,'rockdeng','sadmin','2019-07-03 07:30:31','2018-10-20 04:27:11',1,1),(83,56,'1.1.1','ç›‘æŽ§å‘Šè­¦ç›¸å…³','',0,'sadmin','sadmin','2019-08-28 08:49:36','2019-08-28 08:49:24',1,1);
/*!40000 ALTER TABLE `mt_attr_type` ENABLE KEYS */;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_type_ins after insert on mt_attr_type for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr_type', new.type); end */;;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_type_up after update on mt_attr_type for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr_type', old.type); end */;;
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
  `status` tinyint(4) DEFAULT '0',
  `config_desc` varchar(128) DEFAULT NULL,
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `write_speed` int(10) unsigned DEFAULT '0',
  PRIMARY KEY (`config_id`)
) ENGINE=InnoDB AUTO_INCREMENT=178 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_log_config`
--

LOCK TABLES `mt_log_config` WRITE;
/*!40000 ALTER TABLE `mt_log_config` DISABLE KEYS */;
INSERT INTO `mt_log_config` VALUES (30,26,127,'fcgi_mt_slog',34,'2019-08-26 12:47:52',0,'rockdeng','sadmin',0,'fcgi_mt_slog',1,1,0),(30,25,127,'fcgi_slog_flogin',35,'2019-08-26 12:47:38',0,'rockdeng','sadmin',0,'fcgi_slog_flogin',1,1,0),(30,24,127,'slog_client',36,'2019-08-26 12:47:25',0,'rockdeng','sadmin',0,'slog_client',1,1,0),(30,23,127,'slog_write',37,'2019-08-26 12:47:15',0,'rockdeng','sadmin',0,'slog_write',1,1,0),(30,22,127,'slog_server',38,'2019-08-26 12:47:03',0,'rockdeng','sadmin',0,'slog_server',1,1,0),(30,21,127,'slog_config',39,'2019-08-26 12:46:54',0,'rockdeng','sadmin',0,'slog_config',1,1,0),(30,27,127,'slog_mtreport_server',40,'2019-08-26 12:48:09',0,'rockdeng','sadmin',0,'slog_mtreport_server',1,1,0),(30,28,127,'fcgi_mt_slog_monitor',41,'2019-08-26 12:46:43',0,'rockdeng','sadmin',0,'cgi_monitor',1,1,0),(30,29,127,'fcgi_mt_slog_attr',42,'2019-08-26 12:46:29',0,'rockdeng','sadmin',0,'mt_slog_attr',1,1,0),(30,30,127,'fcgi_mt_slog_machine',43,'2019-08-26 12:48:29',0,'rockdeng','sadmin',0,'fcgi_mt_slog_machine',1,1,0),(30,31,127,'fcgi_mt_slog_view',44,'2019-08-26 12:49:26',0,'rockdeng','sadmin',0,'fcgi_mt_slog_view',1,1,0),(30,33,127,'slog_monitor_server',46,'2019-08-26 12:45:58',0,'rockdeng','sadmin',0,'config_monitor_server',1,1,0),(30,34,127,'fcgi_mt_slog_showview',47,'2019-08-26 12:49:43',0,'rockdeng','sadmin',0,'fcgi_mt_slog_showview',1,1,0),(30,35,127,'fcgi_mt_slog_warn',48,'2019-08-31 11:06:51',0,'rockdeng','sadmin',0,'fcgi_mt_slog_warn',1,1,0),(30,39,127,'fcgi_mt_slog_user',52,'2019-08-31 11:06:59',0,'rockdeng','sadmin',0,'fcgi_mt_slog_user',1,1,222),(30,47,255,'slog_monitor_client',62,'2019-05-24 02:43:17',1501204510,'rockdeng','rockdeng',0,'server attr client',1,1,0),(30,33,127,'slog_monitor_server',63,'2019-08-27 00:14:59',1501219043,'rockdeng','sadmin',0,' attr server',1,1,4200),(30,48,127,'slog_deal_warn',64,'2019-08-26 12:45:20',1502026252,'rockdeng','sadmin',0,'mail',1,1,0),(30,59,127,'slog_check_warn',74,'2019-08-26 12:45:27',1533356355,'rockdeng','sadmin',0,'å‘Šè­¦æ¨¡å—',1,1,0);
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
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'æœºå™¨ç¼–å·',
  `name` varchar(64) NOT NULL COMMENT 'æœºå™¨åç§°',
  `ip1` int(12) unsigned DEFAULT NULL,
  `ip2` int(12) unsigned DEFAULT NULL,
  `ip3` int(12) unsigned DEFAULT NULL,
  `ip4` int(12) unsigned DEFAULT NULL,
  `user_add` varchar(64) DEFAULT 'rock' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'æ·»åŠ æ—¶é—´',
  `user_mod` varchar(64) DEFAULT 'rock' COMMENT 'æœ€åŽæ›´æ–°ç”¨æˆ·',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'æ›´æ–°æ—¶é—´',
  `machine_desc` varchar(256) DEFAULT NULL COMMENT 'æœºå™¨æè¿°',
  `status` tinyint(4) DEFAULT '0' COMMENT 'çŠ¶æ€ç  0-æ­£å¸¸ä½¿ç”¨ 1-åˆ é™¤æ ‡å¿— 2-åœç”¨',
  `warn_flag` int(8) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `model_id` int(8) DEFAULT '0' COMMENT 'æœºå™¨åž‹å·',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  PRIMARY KEY (`id`),
  KEY `ip1` (`ip1`)
) ENGINE=InnoDB AUTO_INCREMENT=115 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_machine`
--

LOCK TABLES `mt_machine` WRITE;
/*!40000 ALTER TABLE `mt_machine` DISABLE KEYS */;
INSERT INTO `mt_machine` VALUES (113,'config',1828720812,4224086138,0,0,'rockdeng','0000-00-00 00:00:00','sadmin','2019-09-03 09:50:31','æ—¥å¿—æœåŠ¡å™¨',1,1,2,1,1),(114,'172.18.67.243',4081259180,3113897848,0,0,'rock','2019-09-03 08:43:52','sadmin','2019-09-03 09:40:05','ç³»ç»Ÿè‡ªåŠ¨æ·»åŠ ',0,1,2,1,1);
/*!40000 ALTER TABLE `mt_machine` ENABLE KEYS */;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_machine_ins after insert on mt_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_machine', new.id); end */;;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_machine_up after update on mt_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_machine', old.id); end */;;
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
  `status` tinyint(4) DEFAULT '0',
  `user_add` varchar(32) NOT NULL,
  `user_mod` varchar(32) NOT NULL,
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  PRIMARY KEY (`module_id`)
) ENGINE=InnoDB AUTO_INCREMENT=150 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_module_info`
--

LOCK TABLES `mt_module_info` WRITE;
/*!40000 ALTER TABLE `mt_module_info` DISABLE KEYS */;
INSERT INTO `mt_module_info` VALUES ('slog_config','å¤„ç†ç›‘æŽ§ç³»ç»Ÿç›¸å…³é…ç½®ï¼Œæœ‰æ›´æ–°æ—¶å°†é…ç½®ä»Žæ•°æ®åº“æ›´æ–°åˆ°å…±äº«å†…å­˜',30,21,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-26 12:50:24',1,1),('slog_server','æ—¥å¿—æœåŠ¡ï¼ŒæŽ¥æ”¶è¿œç¨‹logå¹¶å†™å…¥æœåŠ¡å™¨å…±äº«å†…å­˜ä¸­',30,22,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-26 12:50:17',1,1),('slog_write','æ—¥å¿—å†™å…¥ç£ç›˜æœåŠ¡',30,23,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-26 12:50:12',1,1),('slog_client','ç”¨äºŽæ”¶é›†å¤šæœºéƒ¨ç½²æ—¶ï¼Œç›‘æŽ§ç³»ç»Ÿè‡ªèº«äº§ç”Ÿçš„æ—¥å¿—',30,24,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-26 12:50:07',1,1),('fcgi_slog_flogin','ç”¨äºŽå¤„ç†ç”¨æˆ·ç™»å½•æŽˆæƒ',30,25,0,'rockdeng','rockdeng','0000-00-00 00:00:00','2017-07-28 01:19:56',1,1),('fcgi_mt_slog','ç”¨äºŽå¤„ç†ç³»ç»Ÿæ—¥å¿—å±•ç¤ºï¼Œåº”ç”¨æ¨¡å—é…ç½®ç­‰',30,26,0,'rockdeng','rockdeng','0000-00-00 00:00:00','2014-12-14 04:04:29',1,1),('slog_mtreport_server','ç®¡ç†ç›‘æŽ§ agent slog_mtreport_client çš„æŽ¥å…¥ï¼Œä»¥åŠä¸‹å‘é…ç½®',30,27,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-26 12:44:40',1,1),('fcgi_mt_slog_monitor','ç›‘æŽ§ç³»ç»Ÿä¸»é¡µ',30,28,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-07-02 07:53:44',1,1),('fcgi_mt_slog_attr','ç”¨äºŽç®¡ç†ç›‘æŽ§ç‚¹å’Œç›‘æŽ§ç‚¹ç±»åž‹',30,29,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-26 12:43:33',1,1),('fcgi_mt_slog_machine','ç”¨äºŽç®¡ç†ç³»ç»Ÿæœºå™¨é…ç½®',30,30,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-07-02 07:51:56',1,1),('fcgi_mt_slog_view','ç”¨äºŽå¤„ç†è§†å›¾é…ç½®',30,31,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-07-02 07:51:44',1,1),('slog_monitor_server','ç”¨äºŽå¤„ç†ç›‘æŽ§ç‚¹ä¸ŠæŠ¥',30,33,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-26 12:12:09',1,1),('fcgi_mt_slog_showview','å¤„ç†webç³»ç»Ÿè§†å›¾å±•ç¤º',30,34,0,'rockdeng','rockdeng','0000-00-00 00:00:00','2018-05-23 11:11:49',1,1),('fcgi_mt_slog_warn','å‘Šè­¦é…ç½®',30,35,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-31 11:07:39',1,1),('fcgi_mt_slog_user','ç›‘æŽ§ç³»ç»Ÿç”¨æˆ·ç®¡ç†cgi',30,39,0,'rockdeng','sadmin','0000-00-00 00:00:00','2019-08-31 11:07:26',1,1),('slog_monitor_client','ç›‘æŽ§ç³»ç»Ÿæœ¬èº«çš„ç›‘æŽ§ç‚¹ä¸ŠæŠ¥æœåŠ¡',30,47,0,'rockdeng','sadmin','2017-07-28 01:14:27','2019-08-26 12:11:46',1,1),('slog_deal_warn','ç›‘æŽ§å‘Šè­¦å¤„ç†æ¨¡å—',30,48,0,'rockdeng','sadmin','2017-08-06 13:30:20','2019-08-26 12:11:00',1,1),('slog_check_warn','å‘Šè­¦æ£€æŸ¥æ¨¡å—',30,59,0,'rockdeng','sadmin','2018-08-04 04:18:14','2019-08-26 12:11:26',1,1),('é»˜è®¤æ¨¡å—','ç³»ç»Ÿä¸ºæ‚¨åˆ›å»ºçš„åˆå§‹æ¨¡å—ï¼Œæ‚¨å¯ä»¥ä¿®æ”¹',38,69,0,'ç³»ç»Ÿ','ç³»ç»Ÿ','2018-12-24 15:24:13','2018-12-24 15:24:13',1,1),('dddd','',39,70,0,'testapp','testapp','2018-12-26 11:20:14','2018-12-26 11:20:14',25,25),('å¤–ç½‘','',42,71,0,'testapp2','testapp2','2018-12-26 13:23:42','2018-12-26 13:23:42',28,28);
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
-- Table structure for table `mt_server`
--

DROP TABLE IF EXISTS `mt_server`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_server` (
  `ip` char(20) NOT NULL,
  `port` int(11) unsigned NOT NULL DEFAULT '12345',
  `type` int(8) unsigned NOT NULL DEFAULT '0',
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `sand_box` int(8) unsigned DEFAULT '0',
  `region` int(8) unsigned DEFAULT '0',
  `idc` int(8) unsigned DEFAULT '0',
  `status` int(8) DEFAULT '0',
  `srv_for` text,
  `weight` int(11) unsigned DEFAULT '60000',
  `cfg_seq` int(11) unsigned DEFAULT '1',
  `user_add` varchar(64) NOT NULL,
  `user_mod` varchar(64) NOT NULL,
  `create_time` int(12) unsigned DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `m_desc` varchar(256) DEFAULT NULL,
  PRIMARY KEY (`ip`,`type`),
  KEY `id` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=24 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_server`
--

LOCK TABLES `mt_server` WRITE;
/*!40000 ALTER TABLE `mt_server` DISABLE KEYS */;
INSERT INTO `mt_server` VALUES ('120.79.154.185',28080,1,3,0,0,0,0,'30,117,118',1000,1567504649,'rockdeng','sadmin',1474811820,'2019-09-03 09:57:29','ç»‘å®šåº”ç”¨idï¼Œå¤„ç†æ—¥å¿—ä¸ŠæŠ¥ï¼Œå¯éƒ¨ç½²å¤šå°'),('172.18.67.243',38080,2,6,0,0,0,0,'',1000,1567504630,'rockdeng','sadmin',1475408172,'2019-09-03 09:57:10','å¤„ç†ç›‘æŽ§ç‚¹æ•°æ®ä¸ŠæŠ¥ã€å¯éƒ¨ç½²å¤šå°'),('172.18.67.243',3306,3,4,0,0,0,0,'',1000,1567504619,'rockdeng','sadmin',1475152894,'2019-09-03 09:56:59','mysql ç›‘æŽ§ç‚¹æœåŠ¡å™¨ï¼Œéƒ¨ç½²1å°'),('172.18.67.243',12121,11,23,0,0,0,0,'',1000,1567504613,'sadmin','sadmin',1561962711,'2019-09-03 09:56:53','web æŽ§åˆ¶å°æœåŠ¡å™¨ï¼Œéƒ¨ç½²1å°');
/*!40000 ALTER TABLE `mt_server` ENABLE KEYS */;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_server_ins after insert on mt_server for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_server', new.id); end */;;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_server_up after update on mt_server for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_server', old.id); end */;;
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
) ENGINE=MyISAM AUTO_INCREMENT=38651 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_table_upate_monitor`
--

LOCK TABLES `mt_table_upate_monitor` WRITE;
/*!40000 ALTER TABLE `mt_table_upate_monitor` DISABLE KEYS */;
INSERT INTO `mt_table_upate_monitor` VALUES ('flogin_user',110,0,'2019-09-03 03:33:24',38541),('flogin_user',110,0,'2019-09-03 03:32:24',38540),('flogin_user',110,0,'2019-09-03 03:32:13',38539),('flogin_user',110,0,'2019-09-03 03:32:07',38538),('flogin_user',1,0,'2019-09-03 03:32:00',38537),('flogin_user',109,0,'2019-09-03 03:31:45',38536),('flogin_user',109,0,'2019-09-03 03:31:37',38535),('flogin_user',1,0,'2019-09-03 03:08:13',38534),('flogin_user',110,0,'2019-09-03 07:49:33',38567),('mt_machine',114,0,'2019-09-03 08:43:52',38570),('flogin_user',1,0,'2019-09-03 03:33:28',38542),('flogin_user',110,0,'2019-09-03 03:37:10',38543),('flogin_user',112,0,'2019-09-03 03:37:10',38544),('flogin_user',114,0,'2019-09-03 03:37:10',38545),('flogin_user',116,0,'2019-09-03 03:37:10',38546),('flogin_user',118,0,'2019-09-03 03:37:10',38547),('flogin_user',120,0,'2019-09-03 03:37:10',38548),('flogin_user',122,0,'2019-09-03 03:37:10',38549),('flogin_user',124,0,'2019-09-03 03:37:10',38550),('flogin_user',126,0,'2019-09-03 03:37:10',38551),('flogin_user',128,0,'2019-09-03 03:37:10',38552),('flogin_user',130,0,'2019-09-03 03:37:10',38553),('flogin_user',132,0,'2019-09-03 03:37:10',38554),('flogin_user',134,0,'2019-09-03 03:37:10',38555),('flogin_user',136,0,'2019-09-03 03:37:10',38556),('flogin_user',138,0,'2019-09-03 03:37:10',38557),('flogin_user',140,0,'2019-09-03 03:37:10',38558),('flogin_user',142,0,'2019-09-03 03:37:10',38559),('flogin_user',144,0,'2019-09-03 03:37:10',38560),('flogin_user',146,0,'2019-09-03 03:37:10',38561),('flogin_user',148,0,'2019-09-03 03:37:10',38562),('flogin_user',109,0,'2019-09-03 03:37:24',38563),('flogin_user',110,0,'2019-09-03 03:37:31',38564),('flogin_user',110,0,'2019-09-03 03:38:42',38565),('flogin_user',1,0,'2019-09-03 03:38:51',38566),('flogin_user',110,0,'2019-09-03 07:56:14',38568),('flogin_user',1,0,'2019-09-03 07:56:19',38569),('mt_warn_info',6654,0,'2019-09-03 08:54:00',38571),('mt_view_bmach',23,114,'2019-09-03 08:54:00',38572),('mt_view_bmach',22,114,'2019-09-03 08:54:00',38573),('mt_view_bmach',26,114,'2019-09-03 08:54:00',38574),('flogin_user',1,0,'2019-09-03 08:54:19',38575),('mt_warn_info',6654,0,'2019-09-03 08:55:00',38576),('mt_warn_info',6654,0,'2019-09-03 08:56:00',38577),('mt_warn_info',6654,0,'2019-09-03 08:57:00',38578),('mt_warn_info',6654,0,'2019-09-03 08:58:00',38579),('mt_warn_info',6654,0,'2019-09-03 08:59:00',38580),('mt_warn_info',6654,0,'2019-09-03 09:00:00',38581),('mt_warn_info',6654,0,'2019-09-03 09:01:00',38582),('mt_warn_info',6654,0,'2019-09-03 09:02:00',38583),('mt_warn_info',6654,0,'2019-09-03 09:03:00',38584),('mt_warn_info',6654,0,'2019-09-03 09:04:00',38585),('mt_warn_info',6654,0,'2019-09-03 09:05:00',38586),('mt_warn_info',6654,0,'2019-09-03 09:06:00',38587),('mt_warn_info',6654,0,'2019-09-03 09:07:00',38588),('mt_warn_info',6654,0,'2019-09-03 09:08:00',38589),('mt_warn_info',6654,0,'2019-09-03 09:09:00',38590),('mt_warn_info',6654,0,'2019-09-03 09:10:00',38591),('mt_warn_info',6654,0,'2019-09-03 09:11:00',38592),('mt_warn_info',6654,0,'2019-09-03 09:12:00',38593),('mt_warn_info',6654,0,'2019-09-03 09:13:00',38594),('mt_warn_info',6654,0,'2019-09-03 09:14:00',38595),('mt_warn_info',6654,0,'2019-09-03 09:15:00',38596),('mt_warn_info',6654,0,'2019-09-03 09:16:00',38597),('mt_warn_info',6654,0,'2019-09-03 09:17:00',38598),('mt_warn_info',6654,0,'2019-09-03 09:18:00',38599),('mt_warn_info',6654,0,'2019-09-03 09:19:00',38600),('mt_warn_info',6654,0,'2019-09-03 09:20:00',38601),('mt_warn_info',6654,0,'2019-09-03 09:21:00',38602),('mt_warn_info',6654,0,'2019-09-03 09:22:00',38603),('mt_warn_info',6654,0,'2019-09-03 09:23:00',38604),('mt_warn_info',6654,0,'2019-09-03 09:24:00',38605),('mt_warn_info',6654,0,'2019-09-03 09:25:00',38606),('mt_warn_info',6654,0,'2019-09-03 09:26:00',38607),('mt_warn_info',6654,0,'2019-09-03 09:27:00',38608),('mt_warn_info',6654,0,'2019-09-03 09:28:00',38609),('mt_warn_info',6654,0,'2019-09-03 09:29:00',38610),('mt_warn_info',6654,0,'2019-09-03 09:30:00',38611),('mt_warn_info',6654,0,'2019-09-03 09:31:00',38612),('mt_warn_info',6654,0,'2019-09-03 09:32:00',38613),('mt_warn_info',6654,0,'2019-09-03 09:33:00',38614),('mt_warn_info',6654,0,'2019-09-03 09:34:00',38615),('mt_warn_info',6654,0,'2019-09-03 09:35:00',38616),('mt_warn_info',6654,0,'2019-09-03 09:36:00',38617),('mt_warn_info',6654,0,'2019-09-03 09:37:00',38618),('mt_warn_info',6654,0,'2019-09-03 09:38:00',38619),('mt_warn_info',6654,0,'2019-09-03 09:39:00',38620),('mt_warn_info',6654,0,'2019-09-03 09:40:00',38621),('mt_machine',114,0,'2019-09-03 09:40:05',38622),('mt_warn_info',6654,0,'2019-09-03 09:41:00',38623),('mt_view_battr',22,82,'2019-09-03 09:42:04',38624),('mt_view_battr',22,75,'2019-09-03 09:42:15',38625),('mt_view_battr',22,202,'2019-09-03 09:42:21',38626),('mt_view_battr',23,64,'2019-09-03 09:42:46',38627),('mt_view_battr',23,81,'2019-09-03 09:43:05',38628),('mt_attr',115,0,'2019-09-03 09:43:54',38629),('mt_attr',114,0,'2019-09-03 09:44:01',38630),('mt_attr',109,0,'2019-09-03 09:45:15',38631),('mt_view_battr',26,114,'2019-09-03 09:45:54',38632),('mt_view_battr',26,109,'2019-09-03 09:45:54',38633),('mt_view_battr',26,115,'2019-09-03 09:45:54',38634),('mt_machine',113,0,'2019-09-03 09:50:31',38635),('mt_view_bmach',23,113,'2019-09-03 09:50:31',38636),('mt_view_bmach',22,113,'2019-09-03 09:50:31',38637),('mt_view_bmach',26,113,'2019-09-03 09:50:31',38638),('mt_warn_info',6649,0,'2019-09-03 09:50:31',38639),('mt_warn_info',6650,0,'2019-09-03 09:50:31',38640),('mt_warn_info',6651,0,'2019-09-03 09:50:31',38641),('mt_warn_info',6652,0,'2019-09-03 09:50:31',38642),('mt_server',4,0,'2019-09-03 09:55:52',38643),('mt_server',6,0,'2019-09-03 09:56:15',38644),('mt_server',6,0,'2019-09-03 09:56:29',38645),('mt_server',23,0,'2019-09-03 09:56:53',38646),('mt_server',4,0,'2019-09-03 09:56:59',38647),('mt_server',6,0,'2019-09-03 09:57:10',38648),('mt_server',3,0,'2019-09-03 09:57:29',38649),('flogin_user',1,0,'2019-09-03 09:57:46',38650);
/*!40000 ALTER TABLE `mt_table_upate_monitor` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `mt_view`
--

DROP TABLE IF EXISTS `mt_view`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_view` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'è§†å›¾ç¼–å·',
  `name` varchar(64) NOT NULL COMMENT 'è§†å›¾åç§°',
  `user_add` varchar(64) DEFAULT 'rock' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'æ·»åŠ æ—¶é—´',
  `user_mod` varchar(64) DEFAULT 'rock' COMMENT 'æœ€åŽæ›´æ–°ç”¨æˆ·',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'æ›´æ–°æ—¶é—´',
  `view_desc` varchar(256) DEFAULT NULL COMMENT 'è§†å›¾æè¿°',
  `status` tinyint(4) DEFAULT '0' COMMENT 'çŠ¶æ€ç  0-æ­£å¸¸ä½¿ç”¨ 1-åˆ é™¤æ ‡å¿—',
  `view_flag` int(8) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1000032 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view`
--

LOCK TABLES `mt_view` WRITE;
/*!40000 ALTER TABLE `mt_view` DISABLE KEYS */;
INSERT INTO `mt_view` VALUES (22,'ç›‘æŽ§ç³»ç»Ÿ-å¸¸ç”¨','rockdeng','2014-02-09 05:55:00','rockdeng','2019-05-23 03:17:30','ç›‘æŽ§ç³»ç»Ÿå¸¸ç”¨ä¸ŠæŠ¥',0,1,1,1),(23,'æ—¥å¿—ç³»ç»Ÿ-æ•°æ®','rockdeng','2014-03-28 13:48:58','sadmin','2019-07-03 03:43:57','ä¸€äº›é‡è¦çš„æ—¥å¿—ç³»ç»Ÿæ•°æ®å±žæ€§ä¸ŠæŠ¥',0,1,1,1),(26,'cgi ç›‘æŽ§','rockdeng','2018-10-07 08:10:45','rockdeng','2019-05-23 03:17:00','cgiç›¸å…³ç›‘æŽ§ä¸ŠæŠ¥',0,1,1,1);
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_ins after insert on mt_view for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_view', new.id); end */;;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_up after update on mt_view for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_view', old.id); end */;;
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
  `status` tinyint(4) DEFAULT '0' COMMENT 'çŠ¶æ€ç  0-æ­£å¸¸ä½¿ç”¨ 1-åˆ é™¤æ ‡å¿—',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`view_id`,`attr_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view_battr`
--

LOCK TABLES `mt_view_battr` WRITE;
/*!40000 ALTER TABLE `mt_view_battr` DISABLE KEYS */;
INSERT INTO `mt_view_battr` VALUES (21,62,0,'0000-00-00 00:00:00'),(21,74,0,'0000-00-00 00:00:00'),(21,63,0,'0000-00-00 00:00:00'),(21,56,0,'0000-00-00 00:00:00'),(21,55,0,'0000-00-00 00:00:00'),(21,57,0,'0000-00-00 00:00:00'),(21,64,0,'0000-00-00 00:00:00'),(21,71,0,'0000-00-00 00:00:00'),(21,73,0,'0000-00-00 00:00:00'),(21,58,0,'0000-00-00 00:00:00'),(21,65,0,'0000-00-00 00:00:00'),(26,114,1,'2019-09-03 09:45:54'),(23,70,0,'0000-00-00 00:00:00'),(26,113,0,'0000-00-00 00:00:00'),(23,65,0,'0000-00-00 00:00:00'),(23,93,0,'0000-00-00 00:00:00'),(21,67,0,'0000-00-00 00:00:00'),(21,75,0,'0000-00-00 00:00:00'),(21,59,0,'0000-00-00 00:00:00'),(21,60,0,'0000-00-00 00:00:00'),(21,70,0,'0000-00-00 00:00:00'),(23,73,0,'0000-00-00 00:00:00'),(23,75,0,'0000-00-00 00:00:00'),(21,61,0,'0000-00-00 00:00:00'),(21,66,0,'0000-00-00 00:00:00'),(21,68,0,'0000-00-00 00:00:00'),(21,69,0,'0000-00-00 00:00:00'),(21,72,0,'0000-00-00 00:00:00'),(21,76,0,'0000-00-00 00:00:00'),(23,67,0,'0000-00-00 00:00:00'),(21,77,0,'0000-00-00 00:00:00'),(21,78,0,'0000-00-00 00:00:00'),(21,79,0,'0000-00-00 00:00:00'),(21,80,0,'0000-00-00 00:00:00'),(21,81,0,'0000-00-00 00:00:00'),(21,82,0,'0000-00-00 00:00:00'),(21,83,0,'0000-00-00 00:00:00'),(23,81,1,'2019-09-03 09:43:05'),(26,143,0,'0000-00-00 00:00:00'),(26,112,0,'0000-00-00 00:00:00'),(26,111,0,'0000-00-00 00:00:00'),(26,110,0,'0000-00-00 00:00:00'),(26,146,0,'0000-00-00 00:00:00'),(26,147,0,'0000-00-00 00:00:00'),(26,145,0,'0000-00-00 00:00:00'),(26,108,0,'0000-00-00 00:00:00'),(26,144,0,'0000-00-00 00:00:00'),(26,142,0,'0000-00-00 00:00:00'),(26,109,1,'2019-09-03 09:45:54'),(22,82,1,'2019-09-03 09:42:04'),(26,115,1,'2019-09-03 09:45:54'),(23,62,0,'0000-00-00 00:00:00'),(23,64,1,'2019-09-03 09:42:46'),(23,94,0,'0000-00-00 00:00:00'),(22,67,0,'0000-00-00 00:00:00'),(22,73,0,'0000-00-00 00:00:00'),(22,75,1,'2019-09-03 09:42:15'),(22,202,1,'2019-09-03 09:42:21');
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
  `status` tinyint(4) DEFAULT '0' COMMENT 'çŠ¶æ€ç  0-æ­£å¸¸ä½¿ç”¨ 1-åˆ é™¤æ ‡å¿—',
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
INSERT INTO `mt_view_bmach` VALUES (21,6,0,1,'0000-00-00 00:00:00'),(22,47,0,1,'0000-00-00 00:00:00'),(21,18,0,1,'0000-00-00 00:00:00'),(21,20,0,1,'0000-00-00 00:00:00'),(26,52,0,1,'0000-00-00 00:00:00'),(23,46,0,1,'0000-00-00 00:00:00'),(23,47,0,1,'0000-00-00 00:00:00'),(23,51,0,1,'0000-00-00 00:00:00'),(23,52,0,1,'0000-00-00 00:00:00'),(22,51,0,1,'0000-00-00 00:00:00'),(22,52,0,1,'0000-00-00 00:00:00'),(26,51,0,1,'0000-00-00 00:00:00'),(26,46,0,1,'0000-00-00 00:00:00'),(23,58,0,1,'0000-00-00 00:00:00'),(23,59,0,1,'0000-00-00 00:00:00'),(22,58,0,1,'0000-00-00 00:00:00'),(22,59,0,1,'0000-00-00 00:00:00'),(22,60,0,1,'0000-00-00 00:00:00'),(23,60,0,1,'0000-00-00 00:00:00'),(23,61,0,1,'0000-00-00 00:00:00'),(22,61,0,1,'0000-00-00 00:00:00'),(22,46,0,1,'0000-00-00 00:00:00'),(22,84,0,1,'0000-00-00 00:00:00'),(23,113,1,1,'2019-09-03 09:50:31'),(22,113,1,1,'2019-09-03 09:50:31'),(26,113,1,1,'2019-09-03 09:50:31'),(23,114,0,1,'2019-09-03 08:54:00'),(22,114,0,1,'2019-09-03 08:54:00'),(26,114,0,1,'2019-09-03 08:54:00');
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
  `user_add` varchar(64) DEFAULT 'rock' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'æ·»åŠ æ—¶é—´',
  `reserved1` int(11) DEFAULT '0' COMMENT 'æœºå™¨id æˆ–è€… è§†å›¾id',
  `reserved2` int(11) DEFAULT '0' COMMENT 'ä¿ç•™',
  `reserved3` varchar(32) DEFAULT NULL COMMENT 'ä¿ç•™',
  `reserved4` varchar(32) DEFAULT NULL COMMENT 'ä¿ç•™',
  `status` tinyint(4) DEFAULT '0' COMMENT 'çŠ¶æ€ç  0-æ­£å¸¸ä½¿ç”¨ 1-åˆ é™¤æ ‡å¿—',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`warn_config_id`)
) ENGINE=MyISAM AUTO_INCREMENT=120 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_warn_config`
--

LOCK TABLES `mt_warn_config` WRITE;
/*!40000 ALTER TABLE `mt_warn_config` DISABLE KEYS */;
INSERT INTO `mt_warn_config` VALUES (119,202,17,1,0,0,22,'sadmin','2019-08-28 09:16:58',0,0,NULL,NULL,0,1,1,'0000-00-00 00:00:00');
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
  `status` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`wid`)
) ENGINE=InnoDB AUTO_INCREMENT=6655 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_warn_info`
--

LOCK TABLES `mt_warn_info` WRITE;
/*!40000 ALTER TABLE `mt_warn_info` DISABLE KEYS */;
INSERT INTO `mt_warn_info` VALUES (6053,51,93,5,10,1538644422,9,2,1538661122,280,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6054,23,93,20,10,1538644422,18,2,1538661122,280,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6055,52,98,0,1,1538644442,72,2,1538645342,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6056,52,97,0,4,1538644442,72,2,1538645342,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6057,23,82,2000,1547,1538644442,18,2,1538661122,264,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6058,52,90,0,3,1538644502,72,2,1538661122,63,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6059,51,98,0,7,1538645102,72,2,1538648342,12,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6060,51,97,0,28,1538645102,72,2,1538648342,12,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6061,51,110,0,2,1538645642,72,2,1538647382,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6062,52,189,70,80,1538647262,9,2,1538659862,21,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6063,52,189,70,80,1538660282,41,2,1538660282,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6064,51,93,5,10,1538661182,9,2,1538662622,25,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6065,23,82,2000,780,1538661182,18,2,1538662622,23,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6066,23,93,20,10,1538661182,18,2,1538662622,25,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6067,52,90,0,1,1538661422,72,2,1538662622,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6068,52,189,70,80,1538661722,9,2,1538662562,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6069,23,82,80,100,1538662622,20,2,1538711104,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6070,52,189,70,120,1538662682,9,2,1538726340,256,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6071,51,93,5,11,1538662682,9,2,1538732280,1124,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6072,52,90,0,3,1538662682,72,2,1538732160,204,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6073,51,98,0,1,1538662682,72,2,1538732160,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6074,51,97,0,4,1538662682,72,2,1538732160,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6075,23,93,20,11,1538662682,18,2,1538732280,1124,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6076,23,82,2000,1534,1538662742,18,2,1538732280,1106,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6077,51,110,0,1,1538703362,72,2,1538703362,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6078,52,98,0,2,1538710742,72,2,1538731860,26,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6079,52,97,0,8,1538710742,72,2,1538731860,26,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6080,52,165,80,81,1538711042,9,2,1538711099,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6081,52,183,80,99,1538711042,9,2,1538726340,219,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6082,52,184,70,93,1538711042,9,2,1538726340,219,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6083,51,93,5,8,1538732340,9,2,1538732760,8,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6084,23,82,2000,1421,1538732340,18,2,1538732760,8,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6085,23,93,20,8,1538732340,18,2,1538732760,8,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6086,52,90,0,2,1538732400,72,2,1538732820,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6087,51,98,0,5,1538732580,72,2,1538732700,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6088,51,97,0,20,1538732580,72,2,1538732700,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6089,52,90,0,1,1538733240,72,2,1538818502,246,0,1538814013,1538818611,0,'0000-00-00 00:00:00'),(6090,52,98,0,1,1538733240,72,2,1538814362,17,0,1538814013,1538818611,0,'0000-00-00 00:00:00'),(6091,52,97,0,4,1538733240,72,2,1538814362,17,0,1538814013,1538818611,0,'0000-00-00 00:00:00'),(6092,51,98,0,2,1538733900,72,2,1538803500,13,0,1538814013,1538818611,0,'0000-00-00 00:00:00'),(6093,51,97,0,8,1538733900,72,2,1538803500,13,0,1538814013,1538818611,0,'0000-00-00 00:00:00'),(6094,52,103,0,1,1538798581,72,2,1538798581,1,0,1538814013,1538818611,0,'0000-00-00 00:00:00'),(6095,52,90,0,3,1538818862,72,2,1538887803,68,0,1538819470,1538887811,0,'0000-00-00 00:00:00'),(6096,52,98,0,2,1538819222,72,2,1538887803,5,0,1538819470,1538887811,0,'0000-00-00 00:00:00'),(6097,52,97,0,8,1538819222,72,2,1538887803,5,0,1538819470,1538887811,0,'0000-00-00 00:00:00'),(6098,51,98,0,5,1538821622,72,2,1538884204,5,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6099,51,97,0,20,1538821622,72,2,1538884204,5,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6100,51,110,0,1,1538823362,72,2,1538823362,1,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6101,46,98,0,1,1538826602,72,2,1538884204,37,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6102,46,97,0,4,1538826602,72,2,1538884204,38,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6103,46,116,0,25,1538826602,72,2,1538826602,1,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6104,46,110,0,44,1538826602,72,2,1538826602,1,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6105,46,86,0,4,1538826602,72,2,1538826602,1,0,1538826793,1538887811,0,'0000-00-00 00:00:00'),(6106,22,163,2,10,1538831220,17,2,1538831460,5,0,1538832086,1538832111,0,'0000-00-00 00:00:00'),(6107,22,163,2,9,1538831520,49,2,1538832064,11,0,1538832086,1538832111,0,'0000-00-00 00:00:00'),(6108,22,163,2,10,1538832124,17,2,1538832124,1,0,1538887824,1538888706,0,'0000-00-00 00:00:00'),(6109,22,163,2,10,1538832184,49,3,1538887684,926,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6111,52,90,0,8,1538887863,72,2,1538887983,2,0,1538888016,1538888768,0,'0000-00-00 00:00:00'),(6112,46,98,0,6,1538887923,72,2,1538887983,2,0,1538888016,1538888211,0,'0000-00-00 00:00:00'),(6113,46,97,0,12,1538887923,72,2,1538888343,3,0,1538888016,1538888471,0,'0000-00-00 00:00:00'),(6114,52,98,0,1,1538888223,72,2,1538888403,3,0,1538890892,1538890923,0,'0000-00-00 00:00:00'),(6117,52,97,0,4,1538888343,72,2,1538888403,2,0,1538890892,1538890923,0,'0000-00-00 00:00:00'),(6118,46,98,0,3,1538888343,72,2,1538888343,1,0,1538890892,1538890923,0,'0000-00-00 00:00:00'),(6121,22,163,2,8,1538889003,17,2,1538889603,11,0,1538889117,1538889647,0,'0000-00-00 00:00:00'),(6122,22,163,2,8,1538889663,17,2,1538889663,1,0,1538890877,1538890923,0,'0000-00-00 00:00:00'),(6128,22,163,2,101,1538890820,17,2,1539219003,96,0,1539218930,1542290512,0,'0000-00-00 00:00:00'),(6129,52,90,0,1,1538890860,72,3,1538891940,10,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6130,52,98,0,1,1538890860,72,3,1538890860,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6131,52,97,0,4,1538890860,72,3,1538891940,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6132,46,98,0,3,1538890980,72,3,1538891885,8,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6133,46,97,0,12,1538890980,72,3,1538891885,8,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6134,52,98,0,1,1538891822,72,3,1538891940,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6135,52,90,0,2,1538892660,72,3,1538892720,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6136,52,90,0,2,1538893560,72,3,1538893620,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6137,52,90,0,2,1538894460,72,3,1538894520,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6138,52,90,0,2,1538895360,72,3,1538895420,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6139,52,90,0,2,1538896260,72,3,1538896320,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6140,52,90,0,1,1538897160,72,3,1538897280,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6141,52,90,0,16,1538898060,72,3,1538902260,22,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6142,46,98,0,1,1538899680,72,3,1538900280,11,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6143,46,97,0,4,1538899680,72,3,1538900280,11,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6144,46,86,0,1,1538899800,72,2,1539403801,2,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6145,51,98,0,1,1538900820,72,3,1538900820,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6146,51,97,0,4,1538900820,72,3,1538900820,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6147,52,90,0,3,1538903160,72,3,1538903220,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6148,52,90,0,2,1538904060,72,3,1538904120,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6149,52,90,0,1,1538904840,72,3,1538904840,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6150,52,90,0,2,1538905560,72,3,1538905800,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6151,52,90,0,2,1538906460,72,3,1538907960,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6152,52,90,0,4,1538908620,72,3,1538912041,18,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6153,52,98,0,2,1538909102,72,3,1538909521,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6154,52,97,0,8,1538909102,72,3,1538909521,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6155,46,98,0,2,1538909402,72,3,1538909641,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6156,46,97,0,8,1538909402,72,3,1538909641,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6157,52,98,0,1,1538910961,72,3,1538910961,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6158,52,97,0,4,1538910961,72,3,1538910961,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6159,46,98,0,3,1538910961,72,3,1538911141,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6160,46,97,0,12,1538910961,72,3,1538911141,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6161,52,163,2,84,1538911981,9,2,1542285000,15,0,1541961046,1542290493,0,'0000-00-00 00:00:00'),(6162,52,98,0,4,1538911981,72,3,1538912883,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6163,52,97,0,16,1538911981,72,3,1538912883,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6164,46,98,0,4,1538911981,72,3,1538912701,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6165,46,97,0,16,1538911981,72,3,1538912701,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6166,52,90,0,1,1538912701,72,3,1538913723,6,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6167,52,90,0,1,1538914563,72,3,1538914683,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6168,52,90,0,2,1538915463,72,3,1538915523,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6169,52,90,0,2,1538916363,72,3,1538916423,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6170,52,90,0,2,1538917263,72,3,1538917323,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6171,52,90,0,2,1538918163,72,3,1538918223,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6172,52,90,0,2,1538919063,72,3,1538919123,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6173,52,90,0,2,1538919963,72,3,1538920023,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6174,52,90,0,2,1538920863,72,3,1538920923,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6175,52,90,0,2,1538921763,72,3,1538921823,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6176,52,90,0,2,1538922663,72,3,1538922723,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6177,52,90,0,2,1538923563,72,3,1538923623,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6178,52,90,0,2,1538924463,72,3,1538924523,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6179,52,90,0,2,1538925363,72,3,1538925423,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6180,52,90,0,2,1538926263,72,3,1538926323,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6181,52,90,0,2,1538927163,72,3,1538927223,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6182,52,90,0,1,1538928063,72,3,1538929023,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6183,52,90,0,2,1538929863,72,3,1538929923,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6184,52,90,0,1,1538930763,72,3,1538930943,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6185,52,90,0,2,1538931663,72,3,1538932563,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6186,52,90,0,1,1538933463,72,3,1538935143,8,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6187,52,90,0,2,1538935863,72,3,1538935923,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6188,52,90,0,1,1538936763,72,3,1538936943,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6189,52,90,0,2,1538937663,72,3,1538937723,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6190,52,90,0,3,1538938563,72,3,1538940063,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6191,52,90,0,2,1538940963,72,3,1538941023,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6192,52,90,0,2,1538941863,72,3,1538942763,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6193,52,90,0,2,1538943663,72,3,1538944563,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6194,52,90,0,2,1538945463,72,3,1538946363,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6195,52,90,0,2,1538947263,72,3,1538947323,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6196,52,90,0,2,1538948163,72,3,1538949063,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6197,52,90,0,1,1538949963,72,3,1538951283,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6198,52,90,0,2,1538952063,72,3,1538952963,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6199,52,90,0,2,1538953863,72,3,1538953923,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6200,52,90,0,2,1538954763,72,3,1538954823,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6201,52,90,0,1,1538955663,72,3,1538955783,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6202,52,90,0,2,1538956563,72,3,1538956623,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6203,52,90,0,2,1538957463,72,3,1538957523,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6204,52,90,0,2,1538958363,72,3,1538958423,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6205,52,90,0,1,1538959263,72,3,1538960823,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6206,52,90,0,1,1538961543,72,3,1538963043,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6207,52,90,0,3,1538963763,72,3,1538963823,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6208,52,90,0,2,1538964663,72,3,1538964723,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6209,52,90,0,1,1538965563,72,3,1538965743,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6210,52,90,0,2,1538966463,72,3,1538967963,6,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6211,52,90,0,2,1538968863,72,3,1538968923,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6212,52,90,0,1,1538969763,72,3,1538971323,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6213,52,90,0,2,1538972163,72,3,1538972223,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6214,52,90,0,2,1538973063,72,3,1538974563,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6215,52,90,0,3,1538975463,72,3,1538976363,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6216,52,90,0,2,1538977263,72,3,1538977323,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6217,52,90,0,15,1538977983,72,3,1538979063,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6218,51,98,0,5,1538977983,72,3,1538978163,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6219,51,97,0,20,1538977983,72,3,1538978163,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6220,51,110,0,1,1538977983,72,2,1540610521,42,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6221,52,90,0,2,1538979963,72,3,1538980023,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6222,52,90,0,2,1538980863,72,3,1538980923,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6223,52,90,0,1,1538981763,72,3,1538983503,6,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6224,52,90,0,2,1538984163,72,3,1538984223,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6225,52,90,0,2,1538985063,72,3,1538985123,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6226,52,90,0,1,1538985903,72,3,1538985903,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6227,52,90,0,2,1538986563,72,3,1538986623,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6228,52,90,0,2,1538987463,72,3,1538987523,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6229,52,90,0,2,1538988363,72,3,1538988423,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6230,52,90,0,2,1538989263,72,3,1538991063,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6231,51,98,0,5,1538989743,72,3,1538989743,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6232,51,97,0,20,1538989743,72,3,1538989743,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6233,52,90,0,2,1538991963,72,3,1538992023,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6234,52,90,0,1,1538992863,72,3,1538993043,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6235,52,90,0,2,1538993763,72,3,1538993823,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6236,52,90,0,2,1538994663,72,3,1538996223,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6237,52,90,0,2,1538997063,72,3,1538997123,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6238,52,90,0,2,1538997963,72,3,1538998023,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6239,52,90,0,2,1538998863,72,3,1538998923,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6240,52,90,0,1,1538999583,72,3,1538999583,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6241,52,90,0,2,1539000363,72,3,1539007564,37,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6242,52,98,0,1,1539001144,72,3,1539002644,9,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6243,52,97,0,4,1539001144,72,3,1539002644,9,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6244,52,183,2,33,1539001744,9,2,1539002584,15,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6245,51,98,0,3,1539001744,72,3,1539002464,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6246,51,97,0,12,1539001744,72,3,1539002464,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6247,46,98,0,2,1539002644,72,3,1539002704,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6248,46,97,0,8,1539002644,72,3,1539002704,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6249,46,98,0,3,1539003784,72,3,1539003844,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6250,46,97,0,12,1539003784,72,3,1539003844,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6251,52,98,0,3,1539005164,72,3,1542117483,59,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6252,52,97,0,12,1539005164,72,3,1542117483,58,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6253,52,90,0,6,1539008464,72,3,1539008464,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6254,52,90,0,2,1539009364,72,3,1539009364,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6255,52,90,0,2,1539010264,72,3,1539010264,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6256,52,90,0,4,1539011164,72,3,1539011164,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6257,52,90,0,6,1539012064,72,3,1539012064,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6258,52,90,0,4,1539012964,72,3,1539012964,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6259,52,90,0,2,1539013864,72,3,1539016084,6,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6260,52,90,0,2,1539016864,72,3,1539018124,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6261,52,90,0,4,1539018964,72,3,1539018964,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6262,52,90,0,5,1539019864,72,3,1539019864,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6263,52,90,0,1,1539020524,72,3,1539020524,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6264,52,90,0,1,1539021364,72,3,1539021724,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6265,52,90,0,4,1539022564,72,3,1539022564,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6266,52,90,0,2,1539023464,72,3,1539023464,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6267,52,90,0,5,1539024364,72,3,1539024364,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6268,52,90,0,5,1539025264,72,3,1539025264,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6269,52,90,0,1,1539025924,72,3,1539025924,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6270,52,90,0,5,1539026764,72,3,1539026764,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6271,52,90,0,1,1539027664,72,3,1539028384,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6272,52,90,0,1,1539029164,72,3,1539029224,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6273,52,90,0,4,1539030064,72,3,1539030064,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6274,52,90,0,4,1539030964,72,3,1539030964,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6275,52,90,0,4,1539031864,72,3,1539031864,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6276,52,90,0,1,1539032764,72,3,1539032824,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6277,52,90,0,1,1539033664,72,3,1539034144,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6278,52,90,0,5,1539034864,72,3,1539034864,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6279,52,90,0,1,1539035524,72,3,1539035524,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6280,52,90,0,4,1539036364,72,3,1539036364,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6281,52,90,0,4,1539037264,72,3,1539037264,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6282,52,90,0,4,1539038164,72,3,1539038164,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6283,52,90,0,2,1539039064,72,3,1539039064,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6284,52,90,0,1,1539039964,72,3,1539040024,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6285,52,90,0,4,1539040864,72,3,1539040864,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6286,52,90,0,3,1539041764,72,3,1539041764,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6287,52,90,0,5,1539042664,72,3,1539042664,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6288,52,90,0,1,1539043564,72,3,1539044044,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6289,52,90,0,4,1539044764,72,3,1539044764,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6290,52,90,0,4,1539045664,72,3,1539045664,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6291,52,90,0,1,1539046564,72,3,1539046924,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6292,52,90,0,1,1539047764,72,3,1539047884,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6293,52,90,0,2,1539048664,72,3,1539050464,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6294,51,98,0,8,1539049084,72,3,1539049144,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6295,51,97,0,32,1539049084,72,3,1539049144,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6296,52,90,0,4,1539051364,72,3,1539051364,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6297,52,90,0,4,1539052264,72,3,1539052264,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6298,52,90,0,4,1539053164,72,3,1539053164,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6299,52,90,0,4,1539054064,72,3,1539054064,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6300,52,90,0,4,1539054964,72,3,1539054964,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6301,52,90,0,4,1539055864,72,3,1539055864,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6302,52,90,0,1,1539056644,72,3,1539058864,9,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6303,51,98,0,1,1539056764,72,3,1539056944,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6304,51,97,0,4,1539056764,72,3,1539056944,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6305,52,90,0,1,1539059584,72,3,1539060004,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6306,52,90,0,3,1539060664,72,3,1539060664,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6307,52,90,0,4,1539061564,72,3,1539061564,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6308,52,90,0,1,1539062464,72,3,1539062524,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6309,52,90,0,1,1539063364,72,3,1539063664,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6310,52,90,0,14,1539064564,72,3,1539064924,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6311,51,98,0,1,1539064924,72,3,1539064924,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6312,51,97,0,4,1539064924,72,3,1539064924,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6313,52,90,0,1,1539065584,72,3,1539067084,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6314,51,98,0,1,1539065584,72,3,1539065584,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6315,51,97,0,4,1539065584,72,3,1539065584,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6316,52,90,0,2,1539067864,72,3,1539069064,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6317,52,90,0,2,1539069724,72,3,1539069724,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6318,52,90,0,1,1539070564,72,3,1539070804,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6319,52,90,0,5,1539071464,72,3,1539071464,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6320,52,90,0,4,1539072364,72,3,1539072364,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6321,52,90,0,5,1539073264,72,3,1539073264,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6322,52,90,0,1,1539074104,72,3,1539074104,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6323,52,90,0,1,1539074764,72,3,1539075964,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6324,52,90,0,4,1539076864,72,3,1539076864,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6325,52,90,0,1,1539077764,72,3,1539077944,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6326,52,90,0,1,1539078664,72,3,1539079804,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6327,52,90,0,2,1539080464,72,3,1539080524,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6328,52,90,0,1,1539081364,72,2,1542429180,7,0,1542429552,1542429557,0,'0000-00-00 00:00:00'),(6329,52,90,0,1,1539082924,72,2,1542428520,6,0,1542428549,1542428554,0,'0000-00-00 00:00:00'),(6330,52,90,0,1,1539084724,72,2,1542428340,6,0,1542428388,1542428393,0,'0000-00-00 00:00:00'),(6331,52,90,0,1,1539085624,72,2,1542426600,5,0,1542426755,1542426760,0,'0000-00-00 00:00:00'),(6332,52,90,0,1,1539086524,72,2,1542424320,44,0,1542424264,1542424808,0,'0000-00-00 00:00:00'),(6333,52,90,0,1,1539088924,72,2,1542415560,40,0,1542409873,1542417722,0,'0000-00-00 00:00:00'),(6334,52,90,0,2,1539089824,72,2,1542286320,7656,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6335,46,98,0,56,1539089824,72,0,1556066160,56,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6336,46,97,0,1,1539089824,72,1,1556000100,72,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6337,51,98,0,4,1539091204,72,2,1542201360,136,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6338,51,97,0,16,1539091204,72,2,1542201360,136,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6339,52,88,0,2,1539150663,72,2,1540379461,19,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6341,46,219,0,3,1539173103,72,2,1539266462,6,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6342,46,116,0,1,1539176943,72,2,1539176943,1,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6343,46,110,0,2,1539178143,72,2,1539266522,2,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6344,51,219,0,3,1539178623,72,2,1539220504,5,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6346,52,55,0,3,1539180663,72,2,1539180663,1,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6347,51,163,80,92,1539217563,9,2,1540862160,7,0,1539218930,1540984901,0,'0000-00-00 00:00:00'),(6348,51,79,0,1,1539270722,72,2,1539270722,1,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6349,58,98,0,3,1539271082,72,2,1542203580,46,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6350,58,97,0,12,1539271082,72,2,1542203580,46,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6351,52,74,0,1,1539304802,72,2,1540835940,5,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6352,58,55,0,1,1539325622,72,2,1539325622,1,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6353,58,116,0,10,1539327542,72,2,1539406441,20,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6354,52,116,0,2,1539401101,72,2,1539422220,7,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6355,60,108,0,31,1539607741,72,2,1539607741,1,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6356,51,183,70,70,1540839660,9,2,1540862100,114,0,1540984893,1540984901,0,'0000-00-00 00:00:00'),(6357,51,163,90,97,1541035560,9,2,1541082420,4,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6358,51,55,0,1,1541082300,72,2,1541082300,1,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6359,58,163,80,86,1541216520,9,2,1541216520,1,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6360,58,55,0,1,1541217000,72,2,1541217000,1,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6361,59,164,80,80,1541299560,9,2,1542211620,2,0,1541961046,1542290493,0,'0000-00-00 00:00:00'),(6362,52,74,0,1,1541585940,72,2,1541585940,1,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6363,61,55,0,1,1541942220,72,2,1541942220,1,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6364,52,86,0,38,1541947080,72,2,1541959560,3,0,1541961046,1542290512,0,'0000-00-00 00:00:00'),(6365,52,164,80,100,1541959560,9,2,1542286440,3,0,1541961046,1542290493,0,'0000-00-00 00:00:00'),(6366,58,86,0,1,1542135242,72,2,1542190082,2,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6367,61,86,0,1,1542159662,72,2,1542232200,2,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6368,52,239,0,1,1542192542,72,2,1542192542,1,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6369,52,236,0,1,1542200280,72,2,1542290400,9,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6370,52,165,80,100,1542200400,9,2,1542286440,3,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6371,58,116,0,8,1542204060,72,2,1542204780,8,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6372,58,236,0,1,1542208680,72,2,1542289080,31,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6373,58,90,0,1,1542243660,72,2,1542262080,2,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6374,58,239,0,1,1542243660,72,2,1542262080,2,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6375,59,86,0,1,1542249780,72,2,1542251400,2,0,1542290485,1542290493,0,'0000-00-00 00:00:00'),(6376,52,236,0,1,1542291360,72,2,1542293460,4,0,1542293660,1542293670,0,'0000-00-00 00:00:00'),(6377,58,236,0,1,1542291840,72,2,1542291840,1,0,1542293660,1542293670,0,'0000-00-00 00:00:00'),(6378,58,116,0,2,1542292380,72,2,1542293280,4,0,1542293660,1542293670,0,'0000-00-00 00:00:00'),(6379,59,55,0,3,1542294180,72,2,1542294180,1,0,1542330871,1542332516,0,'0000-00-00 00:00:00'),(6380,52,236,0,1,1542294480,72,2,1542332220,29,0,1542330871,1542332516,0,'0000-00-00 00:00:00'),(6381,59,236,0,1,1542298380,72,2,1542323460,3,0,1542330871,1542332516,0,'0000-00-00 00:00:00'),(6382,58,236,0,1,1542327180,72,2,1542329280,2,0,1542330871,1542332516,0,'0000-00-00 00:00:00'),(6383,61,86,0,1,1542327300,72,2,1542327300,1,0,1542330871,1542332516,0,'0000-00-00 00:00:00'),(6384,59,71,0,2,1542331920,72,2,1542331920,1,0,1542332521,1542332587,0,'0000-00-00 00:00:00'),(6385,52,236,0,4,1542332700,72,2,1542411240,37,0,1542413325,1542413335,0,'0000-00-00 00:00:00'),(6386,59,236,0,1,1542338760,72,2,1542414600,4,0,1542417717,1542417722,0,'0000-00-00 00:00:00'),(6387,52,164,80,100,1542409677,9,2,1542410700,16,0,1542409873,1542413229,0,'0000-00-00 00:00:00'),(6388,52,165,80,100,1542409677,9,2,1542410760,19,0,1542410194,1542413229,0,'0000-00-00 00:00:00'),(6389,52,116,0,1,1542409677,72,2,1542409680,2,0,1542409873,1542413229,0,'0000-00-00 00:00:00'),(6390,52,236,0,2,1542418500,72,2,1542420240,2,0,1542424264,1542424808,0,'0000-00-00 00:00:00'),(6391,61,86,0,1,1542422040,72,2,1542422220,2,0,1542424264,1542424808,0,'0000-00-00 00:00:00'),(6392,52,86,0,1164,1542424740,72,2,1542424740,1,0,1542424803,1542424808,0,'0000-00-00 00:00:00'),(6393,52,67,1,0,1542426280,10,2,1542426280,1,0,1542426396,1542426403,0,'0000-00-00 00:00:00'),(6394,52,86,0,1163,1542426300,72,2,1542426300,1,0,1542426396,1542426403,0,'0000-00-00 00:00:00'),(6395,52,236,0,1,1542426300,72,2,1542426300,1,0,1542426396,1542426403,0,'0000-00-00 00:00:00'),(6396,52,67,1,0,1542429360,10,2,1542429360,1,0,1542429552,1542429557,0,'0000-00-00 00:00:00'),(6397,52,67,1,0,1542429600,10,2,1542429780,2,0,1542429778,1542429783,0,'0000-00-00 00:00:00'),(6398,52,67,1,0,1542429840,10,2,1542431340,25,0,1542429931,1542432012,0,'0000-00-00 00:00:00'),(6399,59,236,0,1,1542432660,72,2,1542433320,2,0,1542436042,1542436048,0,'0000-00-00 00:00:00'),(6400,52,236,0,1,1542432660,72,2,1542432660,1,0,1542436042,1542436048,0,'0000-00-00 00:00:00'),(6401,61,86,0,1,1542432720,72,2,1542432720,1,0,1542436042,1542436048,0,'0000-00-00 00:00:00'),(6402,52,236,0,1,1542440400,72,3,1544639820,735,0,1542446395,NULL,0,'0000-00-00 00:00:00'),(6403,59,236,0,1,1542449820,72,3,1544610060,99,0,1542452227,NULL,0,'0000-00-00 00:00:00'),(6404,61,86,0,1,1542481620,72,2,1542510960,3,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6405,61,116,0,1,1542538380,72,2,1542538380,1,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6406,61,78,0,1,1542538380,72,2,1542538380,1,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6407,61,77,0,1,1542538380,72,2,1542538380,1,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6408,61,79,0,2,1542538380,72,2,1542538380,1,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6409,61,55,0,6,1542538380,72,2,1542538380,1,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6410,61,71,0,2,1542538380,72,2,1542538380,1,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6411,61,76,0,1,1542538380,72,2,1542539460,2,0,1542539500,1542539505,0,'0000-00-00 00:00:00'),(6412,61,71,0,1,1542539760,72,2,1542539760,1,0,1542554074,1542554084,0,'0000-00-00 00:00:00'),(6413,61,116,0,1,1542543480,72,2,1542543480,1,0,1542554074,1542554084,0,'0000-00-00 00:00:00'),(6414,61,55,0,1,1542551100,72,2,1542551100,1,0,1542554074,1542554084,0,'0000-00-00 00:00:00'),(6415,61,236,0,1,1542552240,72,3,1544193720,348,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6416,52,74,0,1,1542595260,72,3,1544609100,4402,0,1542762037,NULL,0,'0000-00-00 00:00:00'),(6417,61,79,0,1,1542630480,72,2,1543064700,3,0,1542771729,1543100693,0,'0000-00-00 00:00:00'),(6418,61,116,0,1,1542633120,72,2,1542633420,3,0,1542771729,1543100705,0,'0000-00-00 00:00:00'),(6419,52,85,0,1,1542653160,72,2,1542653160,1,0,1542771729,1543100705,0,'0000-00-00 00:00:00'),(6420,59,86,0,3273,1542667860,72,2,1543100640,722,0,1542771729,1543100693,0,'0000-00-00 00:00:00'),(6421,61,252,0,1,1542700380,72,3,1543408560,1702,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6422,52,164,80,85,1542716160,9,2,1543052400,2,0,1542771729,1543100693,0,'0000-00-00 00:00:00'),(6423,52,165,80,90,1542716160,9,2,1543099620,5,0,1542771729,1543100693,0,'0000-00-00 00:00:00'),(6424,61,260,0,1,1542719940,72,3,1543100160,1057,0,1542762037,NULL,0,'0000-00-00 00:00:00'),(6425,61,71,0,1,1542726240,72,2,1542726240,1,0,1542771729,1543100705,0,'0000-00-00 00:00:00'),(6426,61,86,0,2348,1542891060,72,2,1543100640,145,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6427,61,235,0,1,1542985200,72,2,1542985200,1,0,1543100682,1543100705,0,'0000-00-00 00:00:00'),(6428,61,103,0,1,1543027980,72,2,1543027980,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6429,61,234,0,1,1543027980,72,2,1543027980,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6430,52,86,0,31492,1543049520,72,2,1543068720,34,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6431,52,71,0,1,1543049520,72,2,1543053540,68,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6432,59,164,80,100,1543049580,9,2,1543050060,8,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6433,52,252,0,3,1543053600,72,3,1543738620,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6434,61,98,0,1,1543064700,72,2,1543064700,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6435,61,77,0,1,1543064700,72,2,1543064700,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6436,58,55,0,1,1543066140,72,2,1543066140,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6437,58,103,0,1,1543066740,72,2,1543066740,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6438,58,234,0,1,1543066740,72,2,1543066740,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6439,52,67,1,0,1543066860,10,2,1543067520,12,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6440,52,222,0,1,1543068840,72,2,1543068840,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6441,52,189,70,71,1543096920,9,2,1543100640,63,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6442,52,186,0,5,1543098540,72,2,1543100640,36,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6443,52,163,80,80,1543099620,9,2,1543099620,1,0,1543100682,1543100693,0,'0000-00-00 00:00:00'),(6444,59,86,0,3639,1543100700,72,2,1543140000,636,0,1543100717,1543140015,0,'0000-00-00 00:00:00'),(6445,52,189,70,70,1543100700,9,2,1543109400,126,0,1543100717,1543140015,0,'0000-00-00 00:00:00'),(6446,52,186,0,3,1543100700,72,2,1543109220,123,0,1543100717,1543140015,0,'0000-00-00 00:00:00'),(6447,61,86,0,229148,1543100700,72,2,1543140000,395,0,1543100717,1543140015,0,'0000-00-00 00:00:00'),(6448,52,163,80,96,1543106400,9,2,1543106460,2,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6449,52,164,80,94,1543106400,9,2,1543106460,2,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6450,52,165,80,100,1543106400,9,2,1543106460,2,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6451,51,183,80,80,1543109460,9,2,1543112340,11,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6452,58,236,0,1,1543115280,72,3,1544612520,262,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6453,61,79,0,1,1543121580,72,2,1543122600,3,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6454,61,103,0,1,1543121580,72,2,1543121580,1,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6455,61,234,0,1,1543121580,72,2,1543121580,1,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6456,52,67,1,0,1543121640,10,2,1543122180,10,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6457,52,86,0,1132,1543121640,72,2,1543130460,148,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6458,60,79,0,1,1543135080,72,2,1543135080,1,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6459,60,55,0,2,1543135140,72,2,1543135140,1,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6460,60,86,0,816211,1543139580,72,2,1543140000,8,0,1543140005,1543140015,0,'0000-00-00 00:00:00'),(6461,59,86,0,2642,1543140060,72,2,1543150980,183,0,1543150901,1543154870,0,'0000-00-00 00:00:00'),(6462,60,86,0,12473,1543140060,72,2,1543143000,50,0,1543150901,1543154870,0,'0000-00-00 00:00:00'),(6463,61,86,0,1,1543140060,72,2,1543144920,51,0,1543150901,1543154870,0,'0000-00-00 00:00:00'),(6464,52,86,0,1536,1543143120,72,2,1543143120,1,0,1543150901,1543154870,0,'0000-00-00 00:00:00'),(6465,52,235,0,1,1543143180,72,2,1543148700,2,0,1543150901,1543154870,0,'0000-00-00 00:00:00'),(6466,51,183,80,80,1543146840,9,2,1543148400,2,0,1543150901,1543154870,0,'0000-00-00 00:00:00'),(6467,60,55,0,1,1543150560,72,2,1543150560,1,0,1543150901,1543154870,0,'0000-00-00 00:00:00'),(6468,59,79,0,1,1543151160,72,2,1543151160,1,0,1543154852,1543154870,0,'0000-00-00 00:00:00'),(6469,61,86,0,1,1543163100,72,2,1543304760,16,0,1543195760,1543328404,0,'0000-00-00 00:00:00'),(6470,59,86,0,1,1543168260,72,2,1543299360,9,0,1543195760,1543328404,0,'0000-00-00 00:00:00'),(6471,59,183,70,71,1543195500,9,2,1543195860,7,0,1543195760,1543328404,0,'0000-00-00 00:00:00'),(6472,51,183,80,80,1543202640,9,2,1543244040,122,0,1543202709,1543328404,0,'0000-00-00 00:00:00'),(6473,52,260,0,16,1543217340,72,3,1543218180,6,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6474,52,164,80,85,1543239840,9,2,1543240500,2,0,1543328394,1543328404,0,'0000-00-00 00:00:00'),(6475,51,87,0,1,1543245900,72,3,1543327440,1360,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6476,51,234,0,5,1543245900,72,2,1543245900,1,0,1543328394,1543328404,0,'0000-00-00 00:00:00'),(6477,58,86,0,1,1543278000,72,2,1543278000,1,0,1543328394,1543328404,0,'0000-00-00 00:00:00'),(6478,61,79,0,1,1543327740,72,2,1543327740,1,0,1543328394,1543328404,0,'0000-00-00 00:00:00'),(6479,52,67,1,0,1543327920,10,2,1543327980,2,0,1543328394,1543328404,0,'0000-00-00 00:00:00'),(6480,52,86,0,41325,1543327920,72,2,1543327980,2,0,1543328394,1543328404,0,'0000-00-00 00:00:00'),(6481,59,71,0,2,1543328580,72,2,1543328580,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6482,59,79,0,1,1543328580,72,2,1543328640,2,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6483,59,76,0,1,1543328580,72,2,1543328580,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6484,59,103,0,2,1543328640,72,2,1543328700,2,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6485,59,234,0,2,1543328640,72,2,1543328700,2,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6486,60,55,0,1,1543329660,72,2,1543410120,3,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6487,52,163,80,95,1543332300,9,2,1543332300,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6488,52,164,80,82,1543332300,9,2,1543417200,3,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6489,52,165,80,87,1543332300,9,2,1543366140,2,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6490,52,235,0,1,1543366020,72,2,1543366020,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6491,52,222,0,1,1543408680,72,2,1543408680,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6492,52,86,0,7480,1543408680,72,2,1543408800,5,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6493,61,79,0,1,1543409820,72,2,1543409820,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6494,61,98,0,1,1543409820,72,2,1543409820,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6495,61,55,0,1,1543409820,72,2,1543409820,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6496,61,103,0,1,1543409940,72,2,1543409940,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6497,61,234,0,1,1543409940,72,2,1543409940,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6498,52,103,0,8,1543410720,72,2,1543410720,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6499,52,234,0,8,1543410720,72,2,1543410720,1,0,1543454162,1543454174,0,'0000-00-00 00:00:00'),(6500,61,109,0,1,1543454280,72,2,1543454280,1,0,1543754462,1544010024,0,'0000-00-00 00:00:00'),(6501,52,164,80,88,1543487160,9,2,1543500540,5,0,1543754462,1544010024,0,'0000-00-00 00:00:00'),(6502,58,86,0,1,1543502820,72,2,1543502820,1,0,1543754462,1544010024,0,'0000-00-00 00:00:00'),(6503,61,235,0,1,1543541700,72,2,1543994160,6,0,1543754462,1544010024,0,'0000-00-00 00:00:00'),(6504,52,85,0,1,1543621620,72,2,1543691940,2,0,1543754462,1544010024,0,'0000-00-00 00:00:00'),(6505,59,183,70,70,1543659900,9,2,1543661520,23,0,1543754462,1544010024,0,'0000-00-00 00:00:00'),(6506,52,189,70,70,1543735980,9,2,1543736100,3,0,1543754462,1544010024,0,'0000-00-00 00:00:00'),(6507,61,116,0,8,1543755240,72,2,1543931520,5,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6508,61,79,0,1,1543755420,72,2,1543755420,1,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6509,61,103,0,1,1543755420,72,2,1543776660,3,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6510,61,234,0,1,1543755420,72,2,1543776660,3,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6511,59,164,80,100,1543762080,9,2,1543933620,3,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6512,60,55,0,1,1543832880,72,2,1543832880,1,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6513,52,165,80,100,1543905660,9,2,1544010000,10,0,1543905760,1544010024,0,'0000-00-00 00:00:00'),(6514,52,86,0,4543,1543905720,72,2,1543905720,1,0,1543905760,1544010024,0,'0000-00-00 00:00:00'),(6515,61,78,0,1,1543927440,72,2,1543927440,1,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6516,61,77,0,1,1543927440,72,2,1543927440,1,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6517,61,83,0,7,1543931460,72,2,1543931520,2,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6518,61,55,0,1,1543931520,72,2,1543931520,1,0,1544009985,1544010024,0,'0000-00-00 00:00:00'),(6519,52,165,80,100,1544010060,9,2,1544405880,29,0,1544010274,1544406269,0,'0000-00-00 00:00:00'),(6520,59,164,80,96,1544013960,9,2,1544155500,5,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6521,61,116,0,1,1544019960,72,2,1544194020,59,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6522,61,83,0,12,1544019960,72,2,1544020020,2,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6523,61,55,0,1,1544020020,72,2,1544020020,1,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6524,52,235,0,1,1544060820,72,2,1544191740,4,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6525,52,86,0,16257,1544104380,72,2,1544104440,2,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6526,58,86,0,2687,1544106060,72,2,1544406120,8,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6527,52,103,0,1,1544137860,72,2,1544137860,1,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6528,52,234,0,1,1544137860,72,2,1544137860,1,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6529,59,260,0,1280,1544190480,72,3,1544194020,60,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6530,59,252,0,2,1544191860,72,3,1544493300,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6531,52,116,0,2,1544405780,72,3,1544619960,7,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6532,58,79,0,1,1544406180,72,2,1544406180,1,0,1544406261,1544406269,0,'0000-00-00 00:00:00'),(6533,59,86,0,1,1544412960,72,3,1544599920,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6534,52,83,0,10,1544426880,72,0,1544531160,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6535,59,83,0,1,1544426880,72,0,1544619780,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6536,59,116,0,1,1544426880,72,3,1544619780,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6537,61,83,0,20,1544426880,72,0,1544619960,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6538,61,116,0,22,1544426880,72,3,1544619960,10,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6539,58,83,0,1,1544426880,72,0,1544619600,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6540,58,116,0,10,1544426880,72,3,1544619540,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6541,51,83,0,26,1544426880,72,0,1544619960,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6542,51,116,0,24,1544426880,72,3,1544619960,5,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6543,60,83,0,10,1544426880,72,0,1544531160,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6544,60,116,0,1,1544426880,72,3,1544619780,4,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6545,58,86,0,746,1544433060,72,3,1544613840,3009,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6546,52,186,0,2,1544446800,72,0,1544448780,34,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6547,59,79,0,1,1544448600,72,0,1544493300,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6548,59,103,0,2,1544448660,72,0,1544493300,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6549,59,234,0,2,1544448660,72,0,1544493300,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6550,59,163,80,90,1544459520,9,0,1544632560,9,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6551,59,164,80,92,1544459520,9,1,1544641440,18,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6552,52,235,0,1,1544488080,72,0,1544613300,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6554,52,67,1,0,1544530920,10,0,1544531040,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6556,61,219,0,1,1544536380,72,0,1544577420,34,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6558,61,108,0,4,1544577180,72,0,1544577240,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6560,52,222,0,1,1544590620,72,0,1544628180,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6562,58,55,0,2,1544617320,72,0,1544617380,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6564,60,78,0,1,1544619780,72,0,1544619780,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6566,52,78,0,1,1544619780,72,0,1544619780,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6568,59,78,0,2,1544619780,72,0,1544619780,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6570,60,77,0,1,1544619780,72,0,1544619780,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6572,60,55,0,1,1544619780,72,0,1544619840,2,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6574,52,77,0,1,1544619780,72,0,1544619780,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6576,59,77,0,1,1544619780,72,0,1544619780,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6577,46,252,0,3,1548337500,72,3,1549025520,990,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6578,46,108,0,2,1548463500,72,1,1548463500,1,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6579,46,236,0,1,1548467820,72,3,1559102100,2406,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6586,79,188,0,7199,1548558540,72,1,1548558540,1,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6587,79,185,0,6388,1548563580,72,1,1548563580,1,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6588,79,202,0,8017,1548573360,72,1,1548573360,1,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6589,81,185,0,2345,1548595380,72,1,1548595380,1,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6590,81,186,0,1074,1548595560,72,1,1548595560,1,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6591,82,164,1,2,1548848520,9,2,1549030020,215,0,1549026450,1549030055,0,'0000-00-00 00:00:00'),(6593,82,164,1,2,1549030080,9,2,1549108260,32,0,1549031115,1549108319,0,'0000-00-00 00:00:00'),(6594,82,166,1,1,1549030140,9,3,1549162200,387,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6596,82,183,2,19,1549030800,9,2,1549108260,20,0,1549031115,1549108319,0,'0000-00-00 00:00:00'),(6597,82,163,2,3,1549031040,9,2,1549108020,11,0,1549031115,1549108319,0,'0000-00-00 00:00:00'),(6598,82,163,2,2,1549108320,9,1,1549161720,65,0,1549108341,NULL,0,'0000-00-00 00:00:00'),(6599,82,164,1,2,1549108320,9,1,1549162200,356,0,1549108341,NULL,0,'0000-00-00 00:00:00'),(6600,82,183,2,19,1549108320,9,1,1549162200,356,0,1549108341,NULL,0,'0000-00-00 00:00:00'),(6602,46,180,0,3,1549108380,72,1,1549108380,1,0,1550401355,NULL,0,'0000-00-00 00:00:00'),(6603,46,86,0,716,1549238400,72,3,1555469880,253,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6606,46,234,0,2,1550971800,72,2,1558410060,1090,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6609,85,188,0,12,1552465020,72,0,1552644480,1350,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6610,46,188,0,1,1552465080,72,0,1553088840,1350,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6611,46,80,0,26,1552744260,72,0,1552822620,28,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6612,46,78,0,1,1552904700,72,0,1552904700,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6613,46,222,0,5,1553081940,72,2,1558312380,25,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6614,46,103,0,51,1553404680,72,0,1554212820,726,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6615,46,104,0,41,1553605140,72,0,1554212820,668,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6616,46,235,0,1,1553995620,72,0,1556437920,53,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6617,46,83,0,5,1554553680,72,0,1554553680,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6618,46,116,0,4,1554553680,72,3,1554553680,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6620,46,55,0,1,1555307400,72,0,1556417880,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6621,46,71,0,1,1555999920,72,2,1557466620,17,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6622,46,163,2,66,1556264160,9,2,1558486800,818,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6623,46,94,20,10,1556590200,10,0,1556606580,151,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6624,22,82,30,50,1556590200,17,0,1556606280,140,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6625,22,67,1000,698,1556602426,18,0,1556604840,6,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6626,22,67,1000,2581,1556607600,17,0,1556609100,22,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6627,46,93,2,11,1556608680,9,2,1556609100,8,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6628,46,236,0,1,1559549340,72,3,1559728200,11,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6629,46,116,0,3,1559696100,72,3,1559696220,3,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6630,46,236,0,1,1559914020,72,3,1559914020,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6631,46,236,0,1,1560130260,72,3,1560323580,17,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6632,46,163,60,95,1560214380,9,2,1560214380,1,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6633,46,236,0,1,1560412020,72,3,1560497100,10,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6634,46,163,60,62,1560482820,9,2,1560482820,1,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6635,46,189,70,100,1560502560,9,2,1560503580,18,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6636,46,188,0,16,1560502560,72,2,1560503580,18,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6637,46,189,70,100,1560682440,9,2,1560682680,5,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6638,46,188,0,14,1560682440,72,2,1560682680,5,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6639,46,189,70,100,1560819300,9,2,1560939360,600,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6640,46,188,0,16,1560819300,72,2,1560939360,600,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6641,46,163,60,62,1560819300,9,2,1560842280,2,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6642,46,236,0,1,1560820620,72,3,1560937740,11,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6643,22,67,50000,882,1560928080,18,2,1560939360,189,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6644,22,82,10000,46,1560931140,18,2,1560939360,138,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6645,22,82,50,56,1560931620,20,2,1560931860,3,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6646,23,93,100,10,1560933780,18,2,1560937680,20,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6647,23,67,80,1074,1560933840,17,2,1560934440,11,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6648,23,94,50,10,1560934260,18,2,1560939360,67,0,1562116802,1562116809,0,'0000-00-00 00:00:00'),(6649,113,55,0,1069,1566864120,72,0,1566983040,1981,0,NULL,NULL,1,'2019-09-03 09:50:31'),(6650,113,57,0,754927,1566864120,72,0,1566983100,1982,0,NULL,NULL,1,'2019-09-03 09:50:31'),(6651,113,260,0,3,1566865380,72,3,1566866040,12,0,NULL,NULL,1,'2019-09-03 09:50:31'),(6652,113,236,0,1,1566877140,72,3,1566973140,7,0,NULL,NULL,1,'2019-09-03 09:50:31'),(6653,114,236,0,101,1567441920,72,3,1567441920,1,0,NULL,NULL,0,'0000-00-00 00:00:00'),(6654,114,260,0,48,1567500840,72,3,1567503660,48,0,NULL,NULL,0,'2019-09-03 09:41:00');
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
  `status` tinyint(4) DEFAULT '0',
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `config_id` (`config_id`)
) ENGINE=InnoDB AUTO_INCREMENT=40 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `test_key_list`
--

LOCK TABLES `test_key_list` WRITE;
/*!40000 ALTER TABLE `test_key_list` DISABLE KEYS */;
INSERT INTO `test_key_list` VALUES (168,'ddddddddddddgggg',22,0,15,'0000-00-00 00:00:00'),(168,'sdds',250,0,26,'0000-00-00 00:00:00'),(63,'351',103,0,38,'0000-00-00 00:00:00'),(52,'3535',33,0,39,'0000-00-00 00:00:00');
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger test_key_list_ins after insert on test_key_list for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('test_key_list', new.id); end */;;
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
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger test_key_list_up after update on test_key_list for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('test_key_list', old.id); end */;;
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

-- Dump completed on 2019-09-03 18:01:21
-- MySQL dump 10.13  Distrib 5.1.41, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: mysql
-- ------------------------------------------------------
-- Server version	5.1.41-3ubuntu12.10

UPDATE user SET Password=PASSWORD('omniplaces') WHERE user='root'; 
FLUSH PRIVILEGES;

TRUNCATE TABLE `user`;
--
-- Dumping data for table `user`
--

LOCK TABLES `user` WRITE;
/*!40000 ALTER TABLE `user` DISABLE KEYS */;
INSERT INTO `user` VALUES ('localhost','root','*313613907B2E5CC5C4A72003B5871B04974420A7','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','','','','',0,0,0,0),('popeye','root','*313613907B2E5CC5C4A72003B5871B04974420A7','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','','','','',0,0,0,0),('127.0.0.1','root','*313613907B2E5CC5C4A72003B5871B04974420A7','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','','','','',0,0,0,0),('localhost','debian-sys-maint','*72E7CF745F4D3CE4E75498DF0229F2AB0B4BF14D','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','','','','',0,0,0,0),('localhost','dummyusername','*A8B8BE3DEA9E76F9D5920006D974A0D47D23A832','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','N','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','Y','','','','',0,0,0,0);
/*!40000 ALTER TABLE `user` ENABLE KEYS */;
UNLOCK TABLES;


-- Dump completed on 2011-12-07 10:40:25

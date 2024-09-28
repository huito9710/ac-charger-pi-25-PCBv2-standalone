<?php
define("dataPath", "/home/pi/upfiles/");
function SaveItemValue()
{
$bSaved =FALSE;
$fp =fopen(dataPath."setting.ini", "r");
if($fp)
{
$ftmp =fopen(dataPath."setting.$", "w");
if($ftmp)
{
while(!feof($fp))
{
$lineTxt =fgets($fp);           //读原行
fwrite($ftmp, $lineTxt);        //写到新文件
$item =trim($lineTxt);
if((substr($item, 0, 1) =="[") &&(substr($item, -1, 1) =="]"))
{
$item =trim($item, "[]");
if(isset($_POST[$item]))
{
if(!feof($fp))          //读旧格原行 ***
{
$lineTxt =fgets($fp);
if(substr($lineTxt, 0, 1) =="[")
{
fwrite($ftmp, $lineTxt);
if(!feof($fp)) fgets($fp);  //读走旧数据
}
}
fwrite($ftmp, $_POST[$item]."\r\n");
$bSaved =TRUE;
}
}
}
fclose($ftmp);
}
fclose($fp);
if($bSaved)
{
if(copy(dataPath."setting.$", dataPath."setting.ini")) unlink(dataPath."setting.$");
}
}
}
function SaveFileText($filename, $text)
{
if(isset($_POST[$text]))
{
file_put_contents(dataPath.$filename, $_POST[$text]);
}
}
?>

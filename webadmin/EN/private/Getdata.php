<?php
define("dataPath", "/home/pi/upfiles/");
define("docPath", "/home/pi/Downloads/");
function GetItemValue($item)
{
$value ="";
$fp =fopen(dataPath."setting.ini", "r");
if($fp)
{
while(!feof($fp))
{
$lineTxt =fgets($fp);
if(strstr($lineTxt, "[".$item."]") !=FALSE)
{
if(!feof($fp))
{
$value =fgets($fp);
if(substr($value, 0, 1) =="[")  //读走旧格式数据 ***
{
$value ="";
if(!feof($fp)) $value =fgets($fp);
}
$value =trim($value);
}
break;
}
}
fclose($fp);
}
return $value;
}
function GetFileContent($filename)
{
$value =file_get_contents(dataPath.$filename);
if($value ==FALSE) $value ="";
return $value;
}
function GetRecordFile($filename)
{
$docfile ="../Downloads/".$filename;
if(!is_file($docfile)) $docfile ="#";
return $docfile;
}
function Readpage($filename, $page, $pageSize)
{
$txtLines =array();
$startlineNo =($page -1)*$pageSize+1;
if(is_file(docPath.$filename))
{
$fp =fopen(docPath.$filename, "r");
if($fp)
{
$i =0;
while(!feof($fp))
{
$text =fgets($fp);
if($i >=$startlineNo)
{
$txtLines[$i -$startlineNo] =$text;
if($i >= $startlineNo+$pageSize) break;
}
$i++;
}
fclose($fp);
}
}
return $txtLines;
}
function copyRecord($filename, $beginTime, $endTime)
{
if(is_file(docPath.$filename))
{
$textlines =file(docPath.$filename);
$beginTime =trim($beginTime);
$beginTime =str_replace(" ", "T", $beginTime);
$endTime =trim($endTime);
$endTime =str_replace(" ", "T", $endTime);
$endTime =$endTime.":60";
for($i=1; $i<count($textlines); $i++)
{
$item =explode(",", $textlines[$i]);
if(count($textlines) >1)
{
$item[1] =trim($item[1]);
if(($item[1] <$beginTime) || ($item[1] >$endTime)) $textlines[$i] ="";
}
else $textlines[$i] ="";
}
file_put_contents("../Downloads/".$filename, $textlines);
}
}
?>

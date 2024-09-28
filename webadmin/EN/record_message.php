<!DOCTYPE html>
<?php
session_start();
if(empty($_SESSION['isLogin']))
{
echo "<script language='javascript'> window.location.href='index.php';</script>";
}
include_once "./private/Getdata.php";
?>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=gb2312" />
<title>Setting</title>
<link href="../css/public.css?v=1.0" rel="stylesheet" type="text/css">
</head>
<script language="javascript">
function UpdatePage(page)
{
window.location.href='record_charging.php?page='+page;
return true;
}
</script>
<body>
<div id="index" >
<div class="indexBox">
<div class="ipage">
<table width="800" border="0" align="center">
<tr>
<td>
<form method="post" name="form1" id="form1" action="#" enctype="multipart/form-data">
<table width="100%" border="1" cellpadding="1" cellspacing="1" class="tableSTYLE" background="images/_BK.jpg">
<tr>
<td height="35" colspan="10" align="center"><span class="thSTYLE">evMega</span></td>
</tr>
<tr>
<td width="100" height="35" align="center"></td>
<td width="75"  height="35" align="center"><a href="base_data.php">Basic</a></td>
<td width="75"  height="35" align="center"><a href="auth_net.php">Authorise</a></td>
<td width="75"  height="35" align="center"><a href="media_type.php">Media</a></td>
<td width="75"  height="35" align="center"><a href="load_balance.php">Load&nbspMgt</a></td>
<td width="75"  height="35" align="center"><a href="IP_setting.php">IP setup</a></td>
<td width="75"  height="35" align="center"><a href="record_charging.php" class="selectedSTYLE">Data</a></td>
<td width="75"  height="35" align="center"><a href="rader_setting.php">Radar</a></td>
<td width="75"  height="35" align="center"><a href="user_psw.php">User</a></td>
<td width="100" height="35" align="center"><a href="update_firmware.php">Update</a></td>
</tr>
<tr>
<td colspan="1" height="360" align="center" valign="top">
<table width="100%" height="20" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td width="100%" height="35" align="left"><a href="record_charging.php" class="indexSTYLE">Charge&nbspLog</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="record_message.php" class="selectedSTYLE">Fault Log</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="record_export.php" class="indexSTYLE">Export Log</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
</table>
</td>
<td colspan="8" height="360" align="center" valign="top">
<table width="100%" height="100%" border="0" align="center">
<tr>
<td width="100%" height="10" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="100%" height="250" align="center" valign="top" class="tbLabelSTYLE">
<table width="95%" height="100%" border="1" cellspacing="0" align="center" valign="top" bgcolor=#FFFFFF>
<tr>
<td width="150" height="20" align="center" class="thSubSTYLE"><?php echo "Fault Time" ?> </td>
<td width="150" height="20" align="center" class="thSubSTYLE"><?php echo "Fault Code" ?> </td>
<td width="300" height="20" align="center" class="thSubSTYLE"><?php echo "Details" ?> </td>
</tr>
<?php
if(empty($_GET['page']))
{
$page =1;
}
else
{
$page =$_GET['page'];
}
if($page <1) $page =1;
$fileLines =Readpage("MsgRecord.csv", $page, 8);
$onePage =1;
if(count($fileLines) <8) $onePage =0;
$i =0;
do{
if($i <count($fileLines))
{
$item =explode(",", $fileLines[$i]);
if(count($item) <4) $item =explode(",", ",,,,,,,,,,");
}
else $item =explode(",", ",,,,,,,,,,");
?>
<tr>
<td width="100" height="20" align="center" class="tbDataSTYLE"><?php echo $item[1] ?> </td>
<td width="100" height="20" align="center" class="tbDataSTYLE"><?php echo $item[2] ?> </td>
<td width="300" height="20" align="center" class="tbDataSTYLE"><?php echo $item[3] ?> </td>
</tr>
<?php
$i++;
}while($i<8)
?>
</table>
</td>
</tr>
<tr>
<td colspan="3" height="60" align="center" valign="middle" class="tbLabelSTYLE">
<input type="button" name="pgup" value="<" id="btn" class="btnSTYLE" onclick=<?php echo "window.location.href='record_message.php?page=".($page>1?($page-1):1)."'" ?> />&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<input type="button" name="pgdn" value=">" id="btn" class="btnSTYLE" onclick=<?php echo "window.location.href='record_message.php?page=".($page+$onePage)."'" ?> />
</td>
</tr>
</table>
</td>
<td colspan="1" height="360" align="center" valign="top">
<table width="100%" height="100%" border="0" align="center">
</table>
</td>
</tr>
<tr>
</tr>
</table>
</form>
</td>
</tr>
</table>
</div>
</div>
</div>
</body>
</html>

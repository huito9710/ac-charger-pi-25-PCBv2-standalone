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
<?php
date_default_timezone_set("Asia/Hong_Kong");
if(isset($_POST['Submit']))
{
if($_POST['Submit'] =="Download Charge Record")
{
copyRecord("ChargeRecord.csv", $_POST['beginTime'], $_POST['endTime']);
header("location: ../Downloads/ChargeRecord.csv");
}
else
{
copyRecord("MsgRecord.csv", $_POST['beginTime'], $_POST['endTime']);
header("location: ../Downloads/MsgRecord.csv");
}
}
?>
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
<td width="100%" height="35" align="left"><a href="record_message.php" class="indexSTYLE">Fault Log</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="record_export.php" class="selectedSTYLE">Export Log</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
</table>
</td>
<td colspan="8" height="360" align="center" valign="top">
<table width="100%" height="30" border="0" align="center">
<tr>
<td colspan="3" width="30" height="18" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="180" height="30" align="center" class="tbLabelSTYLE"> Start Time<br>(YYYY-MM-ddThh:mm) </td>
<td width="180" height="30" align="center" class="tbLabelSTYLE"> End Time<br>(YYYY-MM-ddThh:mm) </td>
</tr>
<tr>
<td width="180" height="30" align="center" class="editSTYLE"> <input name="beginTime" type="text" id="beginTime" size="16" maxlength="16" class="editSTYLE" value="" /> </td>
<td width="180" height="30" align="center" class="editSTYLE"> <input name="endTime" type="text" id="endTime" size="16" maxlength="16" class="editSTYLE" value=<?php echo substr(date(DATE_ATOM), 0, 16); ?> /> </td>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td colspan="3" width="30" height="18" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td colspan="3" width="100%" height="40" align="center" style="padding-left:0px;">
<input type="submit" name="Submit" value="Download Charge Record" id="btn" class="btnSTYLE" style="width:80%;height:90%;background-color:#008080;font-weight:bold;"/>
</td>
</tr>
<tr>
<td colspan="3" width="100%" height="40" align="center" style="padding-left:00px;">
<input type="submit" name="Submit" value="Download Fault Record" id="btn" class="btnSTYLE" style="width:80%;height:90%;background-color:#008080;font-weight:bold;"/>
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

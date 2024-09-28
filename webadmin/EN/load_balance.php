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
function onCheckBoxClick(chkbox, shadow)
{
if(chkbox.checked ==true)
{
shadow.value ="1";
}
else
{
shadow.value ="0";
}
}
</script>
<body>
<div id="index" >
<div class="indexBox">
<div class="ipage">
<table width="800" border="0" align="center">
<tr>
<td>
<form method="post" name="form1" id="form1" action="./private/submit.php" enctype="multipart/form-data">
<table width="100%" border="1" cellpadding="1" cellspacing="1" class="tableSTYLE" background="images/_BK.jpg">
<tr>
<td height="35" colspan="10" align="center"><span class="thSTYLE">evMega</span></td>
</tr>
<tr>
<td width="100" height="35" align="center"></td>
<td width="75"  height="35" align="center"><a href="base_data.php">Basic</a></td>
<td width="75"  height="35" align="center"><a href="auth_net.php">Authorise</a></td>
<td width="75"  height="35" align="center"><a href="media_type.php">Media</a></td>
<td width="75"  height="35" align="center"><a href="load_balance.php" class="selectedSTYLE">Load&nbspMgt</a></td>
<td width="75"  height="35" align="center"><a href="IP_setting.php">IP setup</a></td>
<td width="75"  height="35" align="center"><a href="record_charging.php">Data</a></td>
<td width="75"  height="35" align="center"><a href="rader_setting.php">Radar</a></td>
<td width="75"  height="35" align="center"><a href="user_psw.php">User</a></td>
<td width="100" height="35" align="center"><a href="update_firmware.php">Update</a></td>
</tr>
<tr>
<td colspan="1" height="360" align="center" valign="top">
<table width="100%" height="20" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td width="100%" height="70" align="left"><a href="load_balance.php" class="selectedSTYLE">Load Mgt<br/>Setup</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
</table>
</td>
<td colspan="8" height="360" align="center" valign="top">
<table width="100%" height="30" border="0" align="center">
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Network Fault goto Safe Mode </td>
<input name="safeMode" type="hidden" id="safeMode" value="<?php  $tmp=GetItemValue('safeMode'); echo $tmp; ?>" />
<td width="200" height="30" align="left" class="editSTYLE"> <input name="safeMode_" type="checkbox" id="safeMode_" class="checkboxSTYLE" onClick="onCheckBoxClick(this, safeMode)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Charge with RFID on Netword Fault </td>
<input name="OfflineCharging" type="hidden" id="OfflineCharging" value="<?php  $tmp=GetItemValue('OfflineCharging'); echo $tmp; ?>" />
<td width="200" height="30" align="left" class="editSTYLE"> <input name="OfflineCharging_" type="checkbox" id="OfflineCharging_" class="checkboxSTYLE" onClick="onCheckBoxClick(this, OfflineCharging)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Safe Mode Charge Current </td>
<td width="200" height="30" align="left" class="editSTYLE"> <input name="safeCurrent" type="text" id="safeCurrent" size="20" maxlength="20" class="editSTYLE" value="<?php  echo GetItemValue('safeCurrent'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td colspan="3" height="60" align="right" valign="bottom" class="tbLabelSTYLE" style="padding-right:60px;padding-bottom:5px;"> <input type="reset" name="clean" value="Reset" id="btn" class="btnSTYLE"/>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type="submit" name="Submit" value="Confirm" id="btn" class="btnSTYLE"/> </td>
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

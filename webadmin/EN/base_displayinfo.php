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
<td width="75"  height="35" align="center"><a href="base_data.php" class="selectedSTYLE">Basic</a></td>
<td width="75"  height="35" align="center"><a href="auth_net.php">Authorise</a></td>
<td width="75"  height="35" align="center"><a href="media_type.php">Media</a></td>
<td width="75"  height="35" align="center"><a href="load_balance.php">Load&nbspMgt</a></td>
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
<td width="100%" height="35" align="left"><a href="base_data.php" class="indexSTYLE">Infomation</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_electrical.php" class="indexSTYLE">Electrical</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_login.php" class="indexSTYLE">Password</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_mode.php" class="indexSTYLE">Mode</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_displayinfo.php" class="selectedSTYLE">Display Information</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
</table>
</td>
<td colspan="8" height="360" align="center" valign="top">
<table width="100%" height="30" border="0" align="center">
<tr>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Load Management Message </td>
<input name="LoadManagementMsg" type="hidden" id="LoadManagementMsg" value="<?php  $tmp=GetItemValue('LoadManagementMsg'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="LoadManagementMsgb" type="checkbox" id="LoadManagementMsgb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, LoadManagementMsg)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Charging Time Message </td>
<input name="ChargingTimeMsg" type="hidden" id="ChargingTimeMsg" value="<?php  $tmp=GetItemValue('ChargingTimeMsg'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="ChargingTimeMsgb" type="checkbox" id="ChargingTimeMsgb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, ChargingTimeMsg)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Remain Time Message </td>
<input name="RemainTimeMsg" type="hidden" id="RemainTimeMsg" value="<?php  $tmp=GetItemValue('RemainTimeMsg'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="RemainTimeMsgb" type="checkbox" id="RemainTimeMsgb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, RemainTimeMsg)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Voltage Message </td>
<input name="VoltageMsg" type="hidden" id="VoltageMsg" value="<?php  $tmp=GetItemValue('VoltageMsg'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="VoltageMsgb" type="checkbox" id="VoltageMsgb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, VoltageMsg)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Current Message </td>
<input name="CurrentTimeMsg" type="hidden" id="CurrentTimeMsg" value="<?php  $tmp=GetItemValue('CurrentTimeMsg'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="CurrentTimeMsgb" type="checkbox" id="CurrentTimeMsgb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, CurrentTimeMsg)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Energy Message </td>
<input name="EnergyTimeMsg" type="hidden" id="EnergyTimeMsg" value="<?php  $tmp=GetItemValue('EnergyTimeMsg'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="EnergyTimeMsgb" type="checkbox" id="EnergyTimeMsgb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, EnergyTimeMsg)" <?php if($tmp=="1") echo "checked"?> /> </td>

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

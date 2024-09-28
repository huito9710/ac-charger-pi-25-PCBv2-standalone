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
<td width="75"  height="35" align="center"><a href="auth_net.php" class="selectedSTYLE">Authorise</a></td>
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
<td width="100%" height="35" align="left"><a href="auth_net.php" class="selectedSTYLE">Platform</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="auth_local.php" class="indexSTYLE">Local</a></td>
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
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Mobile APP </td>
<input name="APP" type="hidden" id="APP" value="<?php  $tmp=GetItemValue('APP'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="APP_" type="checkbox" id="APP_" class="checkboxSTYLE" onClick="onCheckBoxClick(this, APP)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Octopus Kiosk </td>
<input name="Octopus" type="hidden" id="Octopus" value="<?php  $tmp=GetItemValue('Octopus'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="Octopus_" type="checkbox" id="Octopus_" class="checkboxSTYLE" onClick="onCheckBoxClick(this, Octopus)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> Charge RFID </td>
<input name="RFID" type="hidden" id="RFID" value="<?php  $tmp=GetItemValue('RFID'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="RFID_" type="checkbox" id="RFID_" class="checkboxSTYLE" onClick="onCheckBoxClick(this, RFID)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> RFID Kiosk </td>
<input name="RFID_PAY" type="hidden" id="RFID_PAY" value="<?php  $tmp=GetItemValue('RFID_PAY'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="RFID_PAY_" type="checkbox" id="RFID_PAY_" class="checkboxSTYLE" onClick="onCheckBoxClick(this, RFID_PAY)" <?php if($tmp=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> No Authorization </td>
<input name="No_Pwd" type="hidden" id="No_Pwd" value="<?php  $tmp=GetItemValue('No_Pwd'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="No_Pwd_" type="checkbox" id="No_Pwd_" class="checkboxSTYLE" onClick="onCheckBoxClick(this, No_Pwd)" <?php if($tmp=="1") echo "checked"?> /> </td>
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

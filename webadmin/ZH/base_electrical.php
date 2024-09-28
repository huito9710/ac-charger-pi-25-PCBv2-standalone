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
<title>充電樁設置</title>
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
<td width="75"  height="35" align="center"><a href="base_data.php" class="selectedSTYLE">基本</a></td>
<td width="75"  height="35" align="center"><a href="auth_net.php">授權</a></td>
<td width="75"  height="35" align="center"><a href="media_type.php">媒體</a></td>
<td width="75"  height="35" align="center"><a href="load_balance.php">負載平衡</a></td>
<td width="75"  height="35" align="center"><a href="IP_setting.php">IP設定</a></td>
<td width="75"  height="35" align="center"><a href="record_charging.php">本機數據</a></td>
<td width="75"  height="35" align="center"><a href="rader_setting.php">雷達</a></td>
<td width="75"  height="35" align="center"><a href="user_psw.php">本機用戶</a></td>
<td width="100" height="35" align="center"><a href="update_firmware.php">軟件更新</a></td>
</tr>
<tr>
<td colspan="1" height="360" align="center" valign="top">
<table width="100%" height="20" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td width="100%" height="35" align="left"><a href="base_data.php" class="indexSTYLE">資料</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_electrical.php" class="selectedSTYLE">電器特性</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_login.php" class="indexSTYLE">登入密碼</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_mode.php" class="indexSTYLE">操作模式</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_displayinfo.php" class="indexSTYLE">顯示信息</a></td>
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
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 最大充電電流 </td>
<td width="390" height="30" align="left" class="editSTYLE"> <input name="maxCurrent" type="text" id="maxCurrent" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('maxCurrent'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 相位 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="PHASE" type="text" id="PHASE" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('PHASE'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 單相顯示 </td>
<input name="singlePhaseDisplay" type="hidden" id="singlePhaseDisplay" value="<?php  echo GetItemValue('singlePhaseDisplay'); ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="singlePhaseDisplayb" type="checkbox" id="singlePhaseDisplayb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, singlePhaseDisplay)" <?php if(GetItemValue('singlePhaseDisplay')=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> CT 比率 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="CTRatio" type="text" id="CTRatio" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('CTRatio'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 插頭 </td>
<td width="360" height="30" align="left" class="editSTYLE"> 
<span>
	<input name="SOCKET" type="radio" id="SOCKET_IEC2" value="0" <?php $curSocketVal=GetItemValue('SOCKET'); if(empty($curSocketVal) || ($curSocketVal == '0')) echo "checked"?> >
	<label for="SOCKET_IEC2">IEC2</label>
</span>
<span style="margin-left:30px">
	<input name="SOCKET" type="radio" id="SOCKET_13A" value="1" <?php if(GetItemValue('SOCKET') == '1') echo "checked"?> >
	<label for="SOCKET_13A">13A &plus; IEC2</label>
</span>
<span style="margin-left:30px">
	<input name="SOCKET" type="radio" id="SOCKET_63A_NL" value="2" <?php if(GetItemValue('SOCKET') == '2') echo "checked"?> >
	<label for="SOCKET_63A_NL">63A No-Lock</label>
</span>
</td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 電量單位:KWh </td>
<input name="Kwh" type="hidden" id="Kwh" value="<?php  echo GetItemValue('Kwh'); ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="Kwhb" type="checkbox" id="Kwhb" class="checkboxSTYLE" onClick="onCheckBoxClick(this, Kwh)" <?php if(GetItemValue('Kwh')=="1") echo "checked"?> /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td colspan="3" height="60" align="right" valign="bottom" class="tbLabelSTYLE" style="padding-right:60px;padding-bottom:5px;"> <input type="reset" name="clean" value="重置" id="btn" class="btnSTYLE"/>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type="submit" name="Submit" value="確認" id="btn" class="btnSTYLE"/> </td>
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

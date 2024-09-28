<?php
session_start();
include_once "./Getdata.php";
$_SESSION['isLogin']  =FALSE;
$user =$_POST['user'];
$pwd  =$_POST['pwd'];
if(($user ==GetItemValue('adminstrator')) &&($pwd ==GetItemValue('adminPwd')))
{
$_SESSION['isLogin']=TRUE;
header("location: ../base_data.php");
}
else
{
unset($_SESSION['isLogin']);
session_destroy();
header("location:../index.php");
}
?>

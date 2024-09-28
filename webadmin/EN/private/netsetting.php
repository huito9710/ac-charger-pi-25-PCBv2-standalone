<?php
session_start();
if(empty($_SESSION['isLogin']))
{
echo "<script language='javascript'> window.location.href='../index.php';</script>";
}
echo "<script language='javascript'>; history.back(-1); </script>";
?>

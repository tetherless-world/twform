<html>
<head>
<style>
fieldset {
 border: 0;
 padding: 0;
 }
</style>
<script src="js/jquery-1.4.4.min.js" type="text/javascript"></script>
<script src="upload.js" type="text/javascript"></script>
</head>
<body>
<form name="twdocsmedia" method="POST" action="upload.php" enctype="multipart/form-data">
  <fieldset>
    <label for="content">Document:</label>
    <input type="file" name="content" class="text ui-widget-content ui-corner-all"/>
   <label for="file">Remote Name:</label>
   <input type="text" name="file" id="file" class="text ui-widget-content ui-corner-all"/>
    <input type="hidden" name="uid" value="<?= $_GET['uid'] ?>"/>
  </fieldset>
</form>
</body>
</html>

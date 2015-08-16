<?php

include_once("TWFormUnitTest.inc");

class TWFormTest extends TWFormUnitTest {
  
  public function test() {
  }
  
};

TWForm::$editing = TRUE;
$form = new TWFormTest();
$form->test();
if($errors) {
  echo "FAILED\n";
}
else {
  echo "SUCCESS\n";
}

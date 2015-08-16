<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

include_once("../../twsparql/twsparql.module");
include_once("../components/TWForm.inc");


class TestPHPTWFormLogger extends PHPTWFormLogger {
  public function logError($str) {
    global $errors;
    parent::logError($str);
    $errors = TRUE;
  }
};

abstract class TWFormUnitTest extends TWForm {
  
  protected $simulate = FALSE;
  protected $altBase = FALSE;
  
  protected function createPage($title, $content) {
    echo "creating page $title\r\n";
    echo "body = $content";
    echo "\r\n\r\n";
  }
  protected function getAction() {
    return "http://localhost/web/test/form";
  }
  protected function getBasePath() {
    return "http://localhost/web/";
  }
  protected function getId() {
    return "1398";
  }
  protected function getInstanceUri() {
    return "http://www.evanpatton.com/evan.rdf#me";
  }
  protected function getLogger() {
    if(TWForm::$logger==NULL) TWForm::$logger = new TestPHPTWFormLogger();
    return TWForm::$logger;
  }
  protected function shouldSimulate() {
    return $this->simulate;
  }
  
  abstract public function test();

  public function getAltBase() {
    return $this->altBase;
  }
};

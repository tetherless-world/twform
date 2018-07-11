<?php

include_once("NewConcept.inc");
include_once("NewEvent.inc");
include_once("NewJournal.inc");
include_once("NewOrganization.inc");
include_once("NewPerson.inc");

abstract class NewInstance {
  protected $uri = "";
  protected $name = "";

  protected function generateURI($prefix) {
    $uri = $prefix;
    $curTime = gettimeofday();
    $uri .= "_".$curTime["sec"]."_".$curTime["usec"];
    $uri = TWSparql::getEngine()->getIbase().$uri;
    $this->uri = $uri;
    return $uri;
  }

  protected function setName($name) {
    $this->name = $name;
  }

  protected function getName() {
    return $this->name;
  }

  abstract public function createNew();
  abstract public function toSPARUL();

  public static function getNewInstance($class, $name) {
    $newObj = null;
    if($class == TWForm::$namespaces["tw"]."Concept") $newObj = new NewConcept();
    else if($class == TWForm::$namespaces["tw"]."Event") $newObj = new NewEvent();
    else if($class == TWForm::$namespaces["tw"]."Journal") $newObj = new NewJournal();
    else if($class == TWForm::$namespaces["foaf"]."Organization") $newObj = new NewOrganization();
    else if($class == TWForm::$namespaces["foaf"]."Person") $newObj = new NewPerson();
    if($newObj) $newObj->setName($name);
  }
};

?>

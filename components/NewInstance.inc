<?php

include_once("NewConcept.inc");
include_once("NewEvent.inc");
include_once("NewJournal.inc");
include_once("NewOrganization.inc");
include_once("NewPerson.inc");

abstract class NewInstance {
  protected $uri = "";
  protected function generateURI($prefix) {
    $uri = $prefix;
    $curTime = gettimeofday();
    $uri .= "_".$curTime["sec"]."_".$curTime["usec"];
    $uri = TWSparql::getEngine()->getIbase().$uri;
    $this->uri = $uri;
    return $uri;
  }

  abstract public function createNew();
  abstract public function toSPARUL();

  public static function getNewInstance($class) {
    TWForm::$namespaces["tw"]."Concept";
    if($class == TWForm::$namespaces["tw"]."Concept") return new NewConcept();
    else if($class == TWForm::$namespaces["tw"]."Event") return new NewEvent();
    else if($class == TWForm::$namespaces["tw"]."Journal") return new NewJournal();
    else if($class == TWForm::$namespaces["foaf"]."Organization") return new NewOrganization();
    else if($class == TWForm::$namespaces["foaf"]."Person") return new NewPerson();
  }
};

?>

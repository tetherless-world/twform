<?php

class NewPerson extends NewInstance {
  public function createNew() {
    return parent::generateURI("Person");
  }

  public function toSPARUL() {
  }

?>


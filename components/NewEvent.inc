<?php

class NewEvent extends NewInstance {
  public function createNew() {
    return parent::generateURI("Event");
  }

  public function toSPARUL() {
  }

?>


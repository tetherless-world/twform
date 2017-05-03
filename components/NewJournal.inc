<?php

class NewJournal extends NewInstance {
  public function createNew() {
    return parent::generateURI("Journal");
  }

  public function toSPARUL() {
  }

?>


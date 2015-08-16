<?php

class TextNode {
  protected $text = NULL;
  
  public function validate() {
    return array();
  }
  
  public function getType() {
    return "textNode";
  }

  protected function __construct($content) {
    $this->text = $content;
  }

  public static function parse($content) {
    return new TextNode($content);
  }

  public function render() {
    return $this->text;
  }

  public function init($space) {
    return "";
  }

  public function render_structure() {
    return "";
  }
};

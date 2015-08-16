<?php


/**
 * Function to convert JSON to RDF
 */
function serializeRDF($format, $arr) {
	if ($format == "ttl") {
		$output = serializeTurtle($arr);
	}
	return $output;
}

function serializeTurtle($arr) {
	$output = "";
	foreach($arr as $s => $pmap) {
		foreach($pmap as $p => $oarr) {
			foreach($oarr as $o) {
				$sb = preg_match("/^_:/",$s); //check if subject is blank
				if ($sb) $output .= "$s <$p> "; 
				else $output .= "<$s> <$p> ";
				
				if ($o['type'] == 'uri') //print URI object
					$output .= '<'.$o['value']."> .\n";
				else if ($o['type'] == 'literal') { //print literal object
					$output .= '"'.$o['value'].'"';
					if (array_key_exists('datatype',$o))
						$output .= '^^<'.$o['datatype'].'>';
					if (array_key_exists('lang',$o))
						$output .= '@'.$o['lang'];
					$output .= " .\n";
				} else if ($o['type'] == 'bnode') { //print blank object
					$output .= $o['value']." .\n";
				}
			}
		}
	}
	return $output;
}

$string = file_get_contents("JSON-test-case.txt");
$arr = json_decode($string, true);
echo serializeRDF("ttl",$arr);

?>
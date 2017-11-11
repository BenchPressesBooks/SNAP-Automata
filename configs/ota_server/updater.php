<?PHP
	header('Content-type: text/plain; charset=utf8', true);

	// Check whether a header is set or contains another value.
	function check_header($name, $value = false) {
		// Verify that the header is set
		if(!isset($_SERVER[$name])) {
			return false;
		}
		// Verify header is set to literally anything other than false or '0'
		if($value && $_SERVER[$name] != $value) {
			return false;
		}

		// Particular header is present and has value
		return true;
	}

	// Update needed, send the binary
	function sendFile($path) {
		// Log actions for review.
		error_log("INFO:  Sending binary '" . $path . "'\n", 3, "updater.log");
		header($_SERVER["SERVER_PROTOCOL"].' 200 OK', true, 200);
		header('Content-Type: application/octet-stream', true);
		header('Content-Disposition: attachment; filename='.basename($path));
		header('Content-Length: '.filesize($path), true);
		header('x-MD5: '.md5_file($path), true);
		readfile($path);
	}

	// Ensures that the correct user agent string is present.
	if(!check_header('HTTP_USER_AGENT', 'ESP8266-http-Update')) {
		// Incorrect user agent. Deny access and send response code.
		header($_SERVER["SERVER_PROTOCOL"].' 403 Forbidden', true, 403);

		// Log actions for review.
		error_log(print_r("ERROR: Unknown user agent " . $_SERVER['HTTP_USER_AGENT'] . "\n", true), 3, "updater.log");
		echo "Script access for given user agent is not allowed. Service only for ESP8266 update polling.\n";
		exit();
	}

	// Ensure the remaining headers are present.
	if( !check_header('HTTP_X_ESP8266_STA_MAC') ||
		!check_header('HTTP_X_ESP8266_AP_MAC') ||
		!check_header('HTTP_X_ESP8266_FREE_SPACE') ||
		!check_header('HTTP_X_ESP8266_SKETCH_SIZE') ||
		!check_header('HTTP_X_ESP8266_SKETCH_MD5') ||
		!check_header('HTTP_X_ESP8266_CHIP_SIZE') ||
		!check_header('HTTP_X_ESP8266_SDK_VERSION')) {
		// One or more required headers are missing, deny access and send response code.
		header($_SERVER["SERVER_PROTOCOL"].' 403 Forbidden', true, 403);

		// Log actions for review.
		error_log(print_r("ERROR: Malformed headers detected. Dumping header array...\n", true), 3, "updater.log");
		error_log(print_r($_SERVER, true), 3, "updater.log");
		error_log(print_r("\n", true), 3, "updater.log");
		echo "Malformed headers detected.\n";
		exit();
	}

	// Include mac address lookup table for providing updates.
	include 'device_map.php';

	// Check that the device trying to pull updates has a mac configured
	if(!isset($db[$_SERVER['HTTP_X_ESP8266_STA_MAC']])) {
		// Log actions for review.
		error_log("ERROR: Updater not configured for MAC " . $_SERVER['HTTP_X_ESP8266_STA_MAC'] . "\n", 3, "updater.log");
		// Send appropriate response code.
		header($_SERVER["SERVER_PROTOCOL"].' 500 Device MAC not configured for updates.', true, 500);
		exit();
	}

	// Produce the binary path given information from the lookup table.
	$localBinary = "bin/".$db[$_SERVER['HTTP_X_ESP8266_STA_MAC']].".bin";

	// Allow any SDK version, it just has to report SOMETHING.
	// Look up MAC in lookup table. If corresponding version matches optional software version from ESP, no update required.
	// If MD5 hash between local binary and ESP8266 binary match, no update required.
	// Otherwise... update is available
	if((!check_header('HTTP_X_ESP8266_SDK_VERSION') && $db[$_SERVER['HTTP_X_ESP8266_STA_MAC']] != $_SERVER['HTTP_X_ESP8266_VERSION']) || $_SERVER["HTTP_X_ESP8266_SKETCH_MD5"] != md5_file($localBinary)) {
		// Update is available for the requesting mac address. Log for later review.
		error_log("INFO:  Update available for device MAC " . $_SERVER['HTTP_X_ESP8266_STA_MAC'] . "\n", 3, "updater.log");
		// Call function to send binary.
		sendFile($localBinary);
	} else {
		// No update is available for the requesting mac address. Log and send appropriate response code.
		error_log("INFO:  No update available for device MAC " . $_SERVER['HTTP_X_ESP8266_STA_MAC'] . "\n", 3, "updater.log");
		header($_SERVER["SERVER_PROTOCOL"].' 304 Not Modified', true, 304);
	}
?>

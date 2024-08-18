<?php

$config = require_once("clock.config.php");

$date = date("Y-m-d H:i:s");
$year = intval(date("Y"));
$month = intval(date("m"));
$day = intval(date("d"));
$weekday = intval(date("w"));
$hour = intval(date("H"));
$minute = intval(date("i"));
$second = intval(date("s"));

$wakeup_alarm_weekday = 0;
$wakeup_alarm_hour = $config["alarm"][0];
$wakeup_alarm_minute = $config["alarm"][1];

$weather = getWeather($config);

$data = [
        "status" => 0,
        "date" => [
                "full" => $date,
                "year" => $year,
                "month" => $month,
                "day" => $day,
                "weekday" => $weekday,
                "hour" => $hour,
                "minute" => $minute,
                "second" => $second,
        ],
        "wakeup_alarm" => [
                "isset" => 1,
                "weekday" => $wakeup_alarm_weekday,
                "hour" => $wakeup_alarm_hour,
                "minute" => $wakeup_alarm_minute,
	],
	"weather" => $weather,
];
$response = json_encode($data)."\n";

echo $response;

function getWeather($config) {
	$lat = $config["lat"];
	$lon = $config["lon"];
	$tz = $config["tz"];
	$url = "https://api.open-meteo.com/v1/forecast?latitude=".$lat."&longitude=".$lon."&current=temperature_2m,wind_speed_10m,precipitation,weather_code&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset,precipitation_sum&timezone=".urlencode($tz);
	//echo $url;
	$weather_raw = file_get_contents($url);
	$wj = json_decode($weather_raw, true);
	$weather = [];
	$code = match ($wj["current"]["weather_code"]) {
		0          => ["Clear sky"],
		1, 2, 3    => ["Mainly clear, partly cloudy"],
		45, 48     => ["Fog and depositing rime fog"],
		51, 53, 55 => ["Drizzle: Light, moderate, and dense"],
		56, 57     => ["Freezing Drizzle: Light and dense"],
		61, 63, 65 => ["Rain: Slight, moderate and heavy"],
		66, 67     => ["Freezing Rain: Light and heavy"],
		71, 73, 75 => ["Snow fall: Slight, moderate, and heavy"],
		77         => ["Snow grains"],
		80, 81, 82 => ["Rain showers: Slight, moderate, and violent"],
		85, 86     => ["Snow showers slight and heavy"],
		95         => ["Thunderstorm: Slight or moderate"],
		96, 99     => ["Thunderstorm with slight and heavy hail"],
	};
	$units = $wj["current_units"];
	$weather["current"] = [
		"precipitation"  => $wj["current"]["precipitation"]." ".$units["precipitation"],
		"temperature_2m" => $wj["current"]["temperature_2m"]." C",
		"wind_speed_10m" => $wj["current"]["wind_speed_10m"]." ".$units["wind_speed_10m"],
		"code_desc" => $code[0],
		//"" => $wj[""]." ".$units[""],
	];
	//return $weather;
	$units = $wj["daily_units"];
	$sunrise = [];
	foreach(array_slice($wj["daily"]["sunrise"], 1, 3) as $v) {
		$sunrise[] = substr($v, 11);
	}
	$sunset = [];
	foreach(array_slice($wj["daily"]["sunset"], 1, 3) as $v) {
		$sunset[] = substr($v, 11);
	}
	$weather["daily"] = [
		"precipitation_sum"  => array_slice($wj["daily"]["precipitation_sum"], 1, 3),
		"temperature_2m_max" => array_slice($wj["daily"]["temperature_2m_max"], 1, 3),
		"temperature_2m_min" => array_slice($wj["daily"]["temperature_2m_min"], 1, 3),
		"sunrise" => $sunrise,
		"sunset" => $sunset,
		//"" => $wj["daily"][""]." ".$units[""],
	];
	return $weather;
}

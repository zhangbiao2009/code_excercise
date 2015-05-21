<?php

define('you', 0);
define('lion', 1);
define('goat', 2);
define('cabbage', 3);

define('left', 0);
define('right', 1);

$start_state = array(
	you => left,
	lion => left,
	goat => left,
	cabbage => left,
);

function is_target_state($state)
{
	return $state[you] == right && $state[lion] == right && $state[goat] == right && $state[cabbage] == right;
}

function state_is_ok($state)
{
	if($state[lion] == $state[goat] && $state[lion] != $state[you])	// lion will eat goat
		return false;
	if($state[cabbage] == $state[goat] && $state[cabbage] != $state[you])	// goat will eat cabbage
		return false;
	return true;
}

$state_arr = array();

function state_is_duplicate($state)
{
	global $state_arr;
	foreach($state_arr as $st){
		if($st[you] == $state[you] && $st[lion] == $state[lion]
			&& $st[goat] == $state[goat]&& $st[cabbage] == $state[cabbage])
			return true;
	}
	return false;
}

function state_add($state)
{
	global $state_arr;
	$state_arr[] = $state;
}

$state_queue = array();

function search($start_state)
{
	global $state_queue;
	array_push($state_queue, $start_state);
	state_add($start_state);

	while(count($state_queue) > 0)
	{
		$state = array_shift($state_queue);
		$child_state = $state;
		$child_state["parent"] = $state;
		$child_state[you] ^= 1; // left to right or right to left

		if(state_is_ok($child_state) && !state_is_duplicate($child_state)) {
			state_add($child_state);		// remember states that arrived
			array_push($state_queue, $child_state);
		}
		for($i=1; $i<4; $i++){	// bring one to travel if possible
			if($child_state[$i] == $child_state[you])	// not on the same side
				continue;
			$child_state[$i] ^= 1;
			if(is_target_state($child_state)){
				echo "found:\n";
				print_r($child_state);
				exit;
			}
			if(state_is_ok($child_state) && !state_is_duplicate($child_state)) {
				state_add($child_state);		// remember states that arrived
				array_push($state_queue, $child_state);
			}
			$child_state[$i] ^= 1;
		}
	}
}

search($start_state);


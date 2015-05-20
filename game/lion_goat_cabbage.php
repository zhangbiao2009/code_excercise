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

$state_stack = array();

function search($state, $d)
{
	global $state_stack;
	array_push($state_stack, $state);
	/*
	echo "d=".$d."\n";
		print_r($state);
	 */
	if(is_target_state($state)){
		echo "found:\n";
		//var_dump($state);
		print_r($state_stack);
		exit;
	}
	if(!state_is_ok($state))
		goto ret;
	if(state_is_duplicate($state))
		goto ret;
	state_add($state);		// remember states that arrived
	$state[you] ^= 1; // left to right or right to left

	for($i=1; $i<4; $i++){	// bring one to travel if possible
		if($state[$i] == $state[you])	// not on the same side
			continue;
		$state[$i] ^= 1;
		search($state, $d+1);
		$state[$i] ^= 1;
	}
	search($state, $d+1);  // travel myself, not bring anyone

	$state[you] ^= 1;
	ret:
		array_pop($state_stack);
		
}

search($start_state, 1);


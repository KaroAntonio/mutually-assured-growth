
var line;
var ctx, canvas;
var ctr = 0;
var r_ctr_max = 1000;
var r_ctr_min = 200;
var pubnub;

var group_state = 0; 
var min_group_state = -255;
var max_group_state = 255;
 
var width;
var height;
var mid;

// background
var bgd_freq = 1;
var bgd_mag = 1;

var params = [];
var active_params = [1];

var update_buffer = 2000;

var session_started = false;
var session_start_time = 0;

var dont_listen = false;

var last_publish = 0;
var publish_interval = 1500;

function setup() {
	$('#loading').hide();

	width = window.innerWidth;
	height = window.innerHeight;
	mid = new p5.Vector(width/2., height/2.)
	
	init_pubnub();

	canvas = createCanvas(width, height).canvas
	$(canvas).css({top:0})

	//ctx = canvas.getContext('2d');

	noiseSeed(5)
	colorMode(HSB)
	init_all_params()

	background('rgba(0,0,0, 1)');
	console.log(0)

	$(document).click(function() {
		console.log('click')
	})
}

function init_all_params() {
	params = [
		init_params(mid.x,mid.y-100, color(200, 0, 255)),
		init_params(mid.x-100,mid.y+50, color(100, 00, 255)),
		init_params(mid.x+100,mid.y+50, color(55, 0, 255)),
		init_params(mid.x+100,mid.y+50, color(55, 0, 255))
	]
}

function draw() {

	if (Date.now() - last_publish > publish_interval) {
		publish_group_state()
		last_publish = Date.now();
	}
	
	ctr += 1

	for (var i = 0; i < params.length; i++) {
		params[i].r_ctr -= 1
		if (params[i].r_ctr <= 0) {
			params[i].rmode0 = params[i].rmode1;
			params[i].rmode1 = choice(range(0,8,1));
			params[i].r_ctr = range_val(r_ctr_min, r_ctr_max);
			
		}
	}
	
	render_background( params )
	
	for (var i=0; i<2000;i++) {
		var p_idx = choice(active_params) 
		if (p_idx) render_point(params[p_idx],i)
	}

}

function render_background( params ) {

	var group_state = get_group_state();

	var distress_h = 0;
	var hype_h = 180;
	var h; 
	var s = (Math.abs(group_state)/255)*30;
	var s = 30;
	var freq = (ctr/(50 - (255-Math.abs(group_state))))
	var b = (Math.cos( freq )+1)/2*40;
	
	if (group_state > 0) {
		h = hype_h;	
	} else {
		h = distress_h
	}

	background(color(h,s,b, 0.03));
	
}

function render_point(prm, i) {

	var theta = (Math.random() * Math.PI*2 + prm.theta_off+prm.r_ctr)% (Math.PI*2)
	var coords = polar_to_cartesian(1, theta )
	var ctr_off = (ctr + prm.ctr_off + r_ctr_max-prm.r_ctr)/10;

	// Randoms
	var rand0 = Math.random()	

	var n0 = noise(rand0) 
	var n1 = noise( coords[0]+1 ,coords[1]+1, ctr_off/300)
	var n2 = noise( coords[0]+1 ,coords[1]+1, ctr_off/50)
	var n3 = noise( (1-(theta/ Math.PI ))*2, ctr_off/300  )  *.5
	
	var w0 = Math.sin( rand0 * Math.PI*4 - prm.dir*(ctr_off)/50  )-1
	var w1 = (Math.sin( ctr_off/50  )+1) * .2
	var w2 = n1
	var w3 = n0
	var w4 = n2
	var w5 = Math.cos(rand0) 

	// Radiuses
	var r;
	var rmode = prm.rmode0
	
	if ( prm.state > 0) {
		// party
		rmode = rmode % 6
	} else {
		rmode = (rmode % 4)+6
	}

	// Circulars
	if (rmode == 0) r = (w0*width*.4) + width*rand0 
	if (rmode == 1) r = (rand0 * width)/2+w0 * width * .08 
	if (rmode == 2) r = ((rand0 * width)/2+w0 * width * .08 + n0*width - width/2) - width/2
	if (rmode == 3) r = w1  * width*2
	if (rmode == 4) r = (w3*.3+w1*.9)* width 
	if (rmode == 5) r = w5  *width/2 

	// Irregulars
	if (rmode == 6) r = w4  *width/2  + w1*width/2
	if (rmode == 7) r = w2  * width
	if (rmode == 8) r = (w1+w4)/2 * width 
	if (rmode == 9) r = (w2+w3)/2 * width 

	if (include([0,1,2,3,7,9], rmode) ) {
		theta = theta + n1*Math.PI
	}
	
	r = r % width/2
	var coords = polar_to_cartesian(r, theta )
	x = coords[0] + prm.cx
	y = coords[1] + prm.cy

	// for testing
	if (0) {
		y = ((r/width)*height)%height
		x = (theta * width)%width
	}

	stroke(prm.c)
	point(x,y);
}

function include(arr,obj) {
    return (arr.indexOf(obj) != -1);
}

function init_pubnub() {
	
	if (!PubNub) return;

	pubnub = new PubNub({
		subscribeKey: "sub-c-d85c438c-d64a-11e7-bcb2-02515ebb3dc0",
		publishKey: "pub-c-6ccec77b-ca42-473d-a464-74d2db31f511",
		ssl: true
	})

	pubnub.addListener({
		status: function(statusEvent) {
			if (statusEvent.category === "PNConnectedCategory") {
				console.log('CONNECTED! I THINIK.')
			} else if (statusEvent.category === "PNUnknownCategory") {
				var newState = {
					new: 'error'
				};
				pubnub.setState({
					state: newState 
				},
			function (status) {
				console.log(statusEvent.errorData.message)
			});
			} 
		},
		message: function(message) {
			parse_message(message)
		}
	})

	pubnub.subscribe({
	    channels: ['Channel0','Channel1','Channel2','Channel3','Channel4','Channel5'],
	});
}

function parse_message(message) {

	//console.log(message)

	var channel = Number(message.channel.slice(7, message.channel.length))
	var msg = message.message;	
	var state = msg.focusDesire;

	if (channel != 0) set_param_state(channel - 1, state)

	get_group_state()
}

function set_param_state(i,state) {

	if (!include(active_params, i))
		active_params.push(i)

	params[i].activate_time = Date.now()
	params[i].state = state;
}

function publish(publishConfig) {

	pubnub.publish(publishConfig, function(status, response) {
	    //console.log(status, response);
	})
}

function publish_begin_session() {
	console.log('publish')
	var publishConfig = {
	    channel : "channel1",
	    message : {"karoMessage":4,"clientMessage":true}
	}
	publish(publishConfig);
}

function publish_end_session() {
	var publishConfig = {
	    channel : "channel1",
	    message : {"karoMessage":5}
	}
	publish(publishConfig);
}

function publish_group_state() {

	var publishConfig = {
	    channel : "Channel0",
	    message : {"state":get_group_state()}
	}

	publish(publishConfig)
}

function get_group_state() {

	if (!active_params.length)
		return 0	
	
	var min_state = 1000000;
	var sum_state = 0;	

	for (var i = 0; i < active_params.length; i++) {
		var p_idx = active_params[i];
		sum_state += params[p_idx].state
		
		if (min_state > params[p_idx].state)
			min_state = params[p_idx].state
	}
	console.log(sum_state, min_state)

	if (min_state < 0) return min_state
	return sum_state / active_params.length

}

function init_params(cx,cy, c) {
	return {
		c : c,
		rmode0 : choice(range(0,8,1)),
		rmode1 : choice(range(0,8,1)),
		r_ctr : range_val(r_ctr_min, r_ctr_max),
		cx : mid.x,
		cy : mid.y,
		ctr_off : range_val(1000,100000),
		theta_off : Math.random() * Math.PI * 2 ,
		distress_lvl : 0,
		activate_time : 0,
		//state : range_val(-255, 255),
		state : 100, 
		dir : 1,
	}
}

function range(s, f, inc) {
	range_arr = [];
	for (var i=s; i < f; i+=inc) {
		range_arr.push(i);
	}
	return range_arr;
}

function range_val(lo, hi) {
	return Math.random() * (hi-lo)+lo
}

function choice(arr) {
	return arr[Math.floor(Math.random() * arr.length)];
}

function rotate_point(center, pos, theta) {
	pos.sub(center)

	var polar = cartesian_to_polar(pos.x, pos.y)
	polar[1] += theta
	var coords = polar_to_cartesian(polar[0], polar[1])
	
	pos.x = coords[0]
	pos.y = coords[1]

	pos.add(center)

	return pos
	
}

function cartesian_to_polar(x,y) {
	var r = (x**2 + y ** 2) ** 0.5
	var theta = Math.atan(y/x);

	if (x<0) theta += Math.PI
	return [r,theta]
}

function polar_to_cartesian(r, theta) {
        var x = r * Math.cos( theta );
        var y = r * Math.sin( theta );
        return [x,y]
}



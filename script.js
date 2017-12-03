
var line;
var ctx, canvas;
var ctr = 0;
var r_ctr_max = 1000;
var r_ctr_min = 200;
var pubnub;
 
var width;
var height;
var mid;

var params = [];

var update_buffer = 2000;

var session_started = false;
var session_start_time = 0;

var dont_listen = false;

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
	init_n_params()

	background('rgba(0,0,0, 1)');
	console.log(0)

	$(document).click(function() {
		console.log('click')
	})
}

function init_n_params() {
	params = [
		init_params(mid.x,mid.y-100, color(200, 0, 255)),
		init_params(mid.x-100,mid.y+50, color(100, 00, 255)),
		init_params(mid.x+100,mid.y+50, color(55, 0, 255))
	]
}

function draw() {
	
	ctr += 1

	for (var i = 0; i < params.length; i++) {
		params[i].r_ctr -= 1
		if (params[i].r_ctr <= 0) {
			params[i].rmode0 = params[i].rmode1;
			params[i].rmode1 = choice(range(0,8,1));
			params[i].r_ctr = range_val(r_ctr_min, r_ctr_max);
			
		}
	}
	
	//distress
	//background(color(0,20,50, 0.03));
	background(color(0,0,10, 0.03));
	
	for (var i=0; i<2000;i++) {
		render_point(params[i%params.length],i)

	}

	
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
	if (Math.random() > prm.r_ctr / r_ctr_max) {
		rmode = prm.rmode0
	} else {
		rmode = prm.rmode0
	}
	//rmode = 5;
	if (rmode == 0) r = (w0*width*.4) + width*rand0
	if (rmode == 1) r = (rand0 * width)/2+w0 * width * .08 
	if (rmode == 2) r = ((rand0 * width)/2+w0 * width * .08 + n0*width - width/2) - width/2
	if (rmode == 3) r = w1  * width*2
	if (rmode == 4) r = w2  * width
	if (rmode == 5) r = (w1+w2)/2 * width 
	if (rmode == 6) r = (w2+w3)/2 * width 
	if (rmode == 7) r = (w3*.3+w1*.9)* width 
	if (rmode == 8) r = w4  *width/2  + w1*width/2
	if (rmode == 9) r = w5  *width/2 
	
	if (include([0,1,2,3,7,9], rmode) ) {
		theta = theta + n1*Math.PI
	}
	
	r = r % width/2
	var coords = polar_to_cartesian(r, theta )
	x = coords[0] + prm.cx
	y = coords[1] + prm.cy

	// for testing
	if (0) {
		x = r
		y = theta * height/2
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
		subscribeKey: "sub-c-ac854de4-c885-11e7-9695-d62da049879f",
		publishKey: "pub-c-31315745-29a7-4863-bc2a-97a528df3c93",
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
	    channels: ['channel1','channel2'],
	});
}

function parse_message(message) {
	console.log(message)	
	var msg = message.message;	
	if (Object.keys(msg)[0] == 'karoMessage' && !msg.hasOwnProperty("clientMessage")){
		if (msg['karoMessage'] == 1){
			p1.on = true;	
		}
		if (msg['karoMessage'] == 0){
			p1.on = false;	
		}
	}

	if (Object.keys(msg)[0] == 'chrisMessage'){
		if (msg['chrisMessage'] == 1){
			p2.on = true;	
		}
		if (msg['chrisMessage'] == 0){
			p2.on = false;	
		}
	}

	if (p1.on && p2.on && !session_started) {
		session_started = true;
		session_start_time = Date.now();
		publish_begin_session()
	}

	
		
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



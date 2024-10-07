let vid_w = 360;
let vid_h = 0;

let camera_activated = false;
let camera_capture_started = false;
let camera_interval = 0;

let video_src = null;
let video_canvas = null;
let video_img = null;
let video_button = null;
let video_status = null;

let camera_context;

let connected = false;
let server_addr = "";
let server_status = null;

let pixel_data = new Uint8Array(176*144*3);

/****** Sets up camera if hardware is available ******/
function setup_camera()
{
	video_src = document.getElementById("campho_video");
	video_canvas = document.getElementById("campho_canvas");
	video_img = document.getElementById("campho_output");
	video_button = document.getElementById("campho_start");
	video_status = document.getElementById("campho_status");
	server_status = document.getElementById("campho_server");

	//Setup camera through getUserMedia
	navigator.mediaDevices.getUserMedia({ video: true, audio: false }).then((stream) =>
	{
		video_src.srcObject = stream;
		video_src.play();
	})

	.catch((err) =>
	{
        	console.error('An error occurred!');
		video_status.innerHTML = "Camera Status : Offline";
	});

	//Once video stream has started set dimensions of video
	video_src.addEventListener("canplay", (ev) =>
	{
		if(!camera_activated)
		{
			vid_h = video_src.videoHeight / (video_src.videoWidth / vid_w);

			if(isNaN(vid_h)) { vid_h = vid_w / (4 / 3); }

			video_src.setAttribute("width", vid_w);
			video_src.setAttribute("height", vid_h);
			video_canvas.setAttribute("width", vid_w);
			video_canvas.setAttribute("height", vid_h);
			camera_activated = true;

			video_status.innerHTML = "Camera Status : Online";
		}
	}, false,);

	//Begin camera capture only after clicking the big button
	video_button.addEventListener("click", (ev) =>
	{
		ev.preventDefault();

		if(!camera_activated) { return; }

		if(!camera_capture_started)
		{
			//Repeat camera capture every 5FPS
			camera_interval = setInterval(get_camera_data, 200);

			camera_capture_started = true;
			video_status.innerHTML = "Camera Status : Stream Started";
			video_button.innerHTML = "End Camera Stream";
		}

		else
		{
			clearInterval(camera_interval);

			camera_capture_started = false;
			video_status.innerHTML = "Camera Status : Online";
			video_button.innerHTML = "Begin Camera Stream";
		}
	}, false,);

	pixel_data.fill(0);

	let req = new XMLHttpRequest();

	req.open("POST", server_addr);
	req.send(null);
}

/****** Grabs 1 frame from camera ******/
function get_camera_data()
{
	//Grab hidden canvas context
	camera_context = video_canvas.getContext("2d");

	//Only update if valid video data was captured
	if(vid_w && vid_h)
	{
		let factor = parseInt(vid_w / 176);

		//Crop canvas to some multiple of 176x144
		video_canvas.width = 176 * factor;
		video_canvas.height = 144 * factor;

		//Crop towards the center of camera input
		let shift_x = -(vid_w - video_canvas.width) / 2;
		let shift_y = -(vid_h - video_canvas.height) / 2;

		camera_context.drawImage(video_src, shift_x, shift_y, vid_w, vid_h);

		let video_data = video_canvas.toDataURL("image/png");
		video_img.setAttribute("src", video_data);

		factor = parseFloat(1.0 / factor); 

		//Scale canvas down to 176x144 for transfer
		camera_context.scale(factor, factor);

		//Convert and send data via networking (websockets)
		convert_pixel_data();
		//send_pixel_data();
	}
}

/****** Convert canvas data into 24-bit pixel data for TCP transfer ******/
function convert_pixel_data()
{
	pixel_data.fill(0);

	let img_data = camera_context.getImageData(0, 0, 176, 144);
  	let temp_data = img.data;

	let r = 0;
	let g = 0;
	let b = 0;

	let index = 0;

	//Cycle through RGBA channels for conversion
	for(let x = 0; x < temp.data.length; x += 4)
	{
		r = (temp_data[x] >> 5);
		g = (temp_data[x + 1] >> 5);
		b = (temp_data[x + 2] >> 5);

		//Send data as 24-bit RGB values
		pixel_data[index++] = b;
		pixel_data[index++] = g;
		pixel_data[index++] = r;
	} 
}

/****** Sends 24-bit pixel data over TCP via websockets ******/
function send_pixel_data()
{
	let req = new XMLHttpRequest();

	req.open("POST", server_addr);
	req.send(null);
}

window.addEventListener("load", setup_camera, false); 

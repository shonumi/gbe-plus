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
let server_ready = true;

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
		video_status.innerHTML = "<span class='e'>📷</span> Camera Status : Offline";
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

			video_status.innerHTML = "<span class='e'>📷</span> Camera Status : Online";
			video_button.disabled = false;
		}
	}, false,);

	//Begin camera capture only after clicking the big button
	video_button.addEventListener("click", (ev) =>
	{
		ev.preventDefault();

		if(!camera_capture_started)
		{
			//Repeat camera capture every 5FPS
			camera_interval = setInterval(get_camera_data, 200);

			camera_capture_started = true;
			video_status.innerHTML = "<span class='e'>📷</span> Camera Status : Stream Started";
			video_button.innerHTML = "End Camera Stream";
		}

		else
		{
			clearInterval(camera_interval);

			camera_capture_started = false;
			video_status.innerHTML = "<span class='e'>📷</span> Camera Status : Online";
			video_button.innerHTML = "Begin Camera Stream";
			server_status.innerHTML = "<span class='e'>🖥️</span> Server Status : Connecting...";
		}
	}, false,);
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

		//Scale image back to canvas
		video_canvas.width = 176;
		video_canvas.height = 144;
		camera_context.drawImage(video_img, 0, 0, 176, 144);

		//Convert and send data via networking (HTTP POST)
		convert_pixel_data();
		send_pixel_data();
	}
}

/****** Convert canvas data into 24-bit pixel data for TCP transfer ******/
function convert_pixel_data()
{
	let img = camera_context.getImageData(0, 0, 176, 144);
	let data = img.data;

	let index = 0;

	//Cycle through RGBA channels for data
	for(let x = 0; x < data.length; x += 4)
	{
		//Send data as 24-bit RGB values
		pixel_data[index++] = data[x + 2];
		pixel_data[index++] = data[x + 1];
		pixel_data[index++] = data[x];
	}
}

/****** Sends 24-bit pixel data over TCP via websockets ******/
function send_pixel_data()
{
	//Only send if the previous request has been confirmed as completed
	//This means the actual video stream may update at less than 5FPS
	//That depends on the server (GBE+), but that's fine, let the server do its thing
	if(server_ready)
	{
		server_ready = false;
		server_addr = "http://" + document.getElementById("campho_address").value;

		fetch(server_addr, 
		{
			method: 'POST',
			body: pixel_data,
		})

		.then(response =>
		{
			server_ready = true;
			server_status.innerHTML = "<span class='e'>🖥️</span> Server Status : Connected";
		})

		.catch((err) =>
		{
        		console.error('A fetch error occurred!');
			server_ready = true;
			server_status.innerHTML = "<span class='e'>🖥️</span> Server Status : Comms Error!";
		});
	}
}

window.addEventListener("load", setup_camera, false); 

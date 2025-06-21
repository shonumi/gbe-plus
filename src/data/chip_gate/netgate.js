const bcg_list = new Array("Cannon", "Hi Cannon", "Mega Cannon", "Air Shot", "Blizzard", "Heat Breath", "Silence", "Tornado", "Wide Shot 1", "Wide Shot 2", "Wide Shot 3", "Flame Line 1", "Flame Line 2", "Flame Line 3", "Vulcan 1", "Vulcan 2", "Vulcan 3", "Spreader", "Heat Shot", "Heat V", "Heat Side", "Bubbler", "Bubble V", "Bubble Side", "Element Flare", "Element Ice", "Static", "Life Sync", "Mini Boomer", "Energy Bomb", "Mega Energy Bomb", "Gun del Sol 1", "Gun del Sol 2", "Gun del Sol 3", "Mag Bolt 1", "Mag Bolt 2", "Mag Bolt 3", "Binder 1", "Binder 2", "Binder 3", "Bug Bomb", "Elec Shock", "Wood Powder", "Cannonball", "Geyser", "Black Bomb", "Sand Ring", "Sword", "Wide Sword", "Long Sword", "Wide Blade", "Long Blade", "Wind Racket", "Custom Sword", "Variable Sword", "Slasher", "Thunderball 1", "Thunderball 2", "Thunderball 3", "Counter 1", "Counter 2", "Counter 3", "Air Hockey 1", "Air Hockey 2", "Air Hockey 3", "Circle Gun 1", "Circle Gun 2", "Circle Gun 3", "Twin Fang 1", "Twin Fang 2", "Twin Fang 3", "White Web 1", "White Web 2", "White Web 3", "Boomerang 1", "Boomerang 2", "Boomerang 3", "Side Bamboo 1", "Side Bamboo 2", "Side Bamboo 3", "Lance", "Hole", "Boy's Bomb 1", "Boy's Bomb 2", "Boy's Bomb 3", "Guard 1", "Guard 2", "Guard 3", "Magnum", "Grab Gel", "Snake", "Time Bomb", "Mine", "Rock Cube", "Fanfare", "Discord", "Timpani", "VDoll", "Big Hammer 1", "Big Hammer 2", "Big Hammer 3", "Grab Revenge", "Grab Banish", "Geddon 1", "Geddon 2", "Geddon 3", "Element Leaf", "Color Point", "Element Sand", "Moko Rush 1", "Moko Rush 2", "Moko Rush 3", "North Wind", "Anti Fire", "Anti Water", "Anti Electric", "Anti Wood", "Anti Navi", "Anti Damage", "Anti Sword", "Anti Recover", "Copy Damage", "Attack +10", "Navi +20", "Roll Arrow 1", "Roll Arrow 2", "Roll Arrow 3", "Guts Punch 1", "Guts Punch 2", "Guts Punch 3", "Propeller Bomb 1", "Propeller Bomb 2", "Propeller Bomb 3", "Search Bomb 1", "Search Bomb 2", "Search Bomb 3", "Meteors 1", "Meteors 2", "Meteors 3", "Lightning 1", "Lightning 2", "Lightning 3", "Hawk Cut 1", "Hawk Cut 2", "Hawk Cut 3", "Number Ball 1", "Number Ball 2", "Number Ball 3", "Metal Gear 1", "Metal Gear 2", "Metal Gear 3", "Panel Shoot 1", "Panel Shoot 2", "Panel Shoot 3", "Aqua Upper 1", "Aqua Upper 2", "Aqua Upper 3", "Green Wood 1", "Green Wood 2", "Green Wood 3", "Muramasa", "Guardian", "Anubis", "Double Point", "Full Custom", "Shooting Star", "Bug Chain", "Jealousy", "Element Dark", "Black Wing", "God Hammer", "Dark Line", "Neo Variable", "Z Saber", "Gun del Sol EX", "Super Vulcan", "Roll", "Roll SP", "Roll DS", "GutsMan", "GutsMan SP", "GutsMan DS", "WindMan", "WindMan SP", "WindMan DS", "SearchMan", "SearchMan SP", "SearchMan DS", "FireMan", "FireMan SP", "FireMan DS", "ThunderMan", "ThunderMan SP", "ThunderMan DS", "ProtoMan", "ProtoMan SP", "ProtoMan DS", "NumberMan", "NumberMan SP", "NumberMan DS", "MetalMan", "MetalMan SP", "MetalMan DS", "JunkMan", "JunkMan SP", "JunkMan DS", "AquaMan", "AquaMan SP", "AquaMan DS", "WoodMan", "WoodMan SP", "WoodMan DS", "TopMan", "TopMan SP", "TopMan DS", "ShadeMan", "ShadeMan SP", "ShadeMan DS", "BurnerMan", "BurnerMan SP", "BurnerMan DS", "ColdMan", "ColdMan SP", "ColdMan DS", "SparkMan", "SparkMan SP", "SparkMan DS", "LaserMan", "LaserMan SP", "LaserMan DS", "KendoMan", "KendoMan SP", "KendoMan DS", "VideoMan", "VideoMan SP", "VideoMan DS", "Marking", "Cannon Mode", "Cannonball Mode", "Sword Mode", "Fire Plus", "Thunder Plus", "Aqua Power", "Wood Power", "Black Weapon", "Final Gun", "", "", "", "", "", "Bass", "Delta Ray", "Bug Curse", "Red Sun", "Bass Another", "Holy Dream", "Blue Moon", "Bug Charge", "", "", "Wind", "Fan", "Crack Out", "Double Crack", "Triple Crack", "Recovery 10", "Recovery 30", "Recovery 50", "Recovery 80", "Recovery 120", "Recovery 150", "Recovery 200", "Recovery 300", "Repair", "Panel Grab", "Area Grab", "Slow Gauge", "Fast Gauge", "Panel Return", "Blinder", "Pop Up", "Invisible", "Barrier", "Barrier 100", "Barrier 200", "Holy Panel", "", "Life Aura", "Attack +30", "Bug Fix", "Sanctuary", "Signal Red", "Black Barrier", "MegaMan Navi", "Roll Navi", "GutsMan Navi", "WindMan Navi", "SearchMan Navi", "FireMan Navi", "ThunderMan Navi", "ProtoMan Navi", "NumberMan Navi", "MetalMan Navi", "JunkMan Navi", "AquaMan Navi", "WoodMan Navi");

const pcg_list = new Array("Cannon", "HiCannon", "MegaCannon", "AirShot", "AirHockey", "Boomer", "Silence", "Tornado", "WideShot1", "WideShot2", "WideShot3", "MarkCannon1", "MarkCannon2", "MarkCannon3", "Vulcan1", "Vulcan2", "Vulcan3", "Spreader", "ThunderBall", "IceSeed", "Pulsar1", "Pulsar2", "Pulsar3", "SpaceShake1", "SpaceShake2", "SpaceShake3", "Static", "LifeSync", "MiniBomb", "EnergyBomb", "MegaEnergyBomb", "GunDelSol1", "GunDelSol2", "GunDelSol3", "Quake1", "Quake2", "Quake3", "CrackBomb", "ParalyzeBomb", "ResetBomb", "BugBomb", "GrassSeed", "LavaSeed", "CannonBall", "Geyser", "BlackBomb", "SeaSeed", "Sword", "WideSword", "LongSword", "WideBlade", "LongBlade", "WindRacket", "CustomSword", "VariableSword", "Slasher", "MoonBlade1", "MoonBlade2", "MoonBlade3", "Katana1", "Katana2", "Katana3", "TankCannon1", "TankCannon2", "TankCannon3", "RedFruit1", "RedFruit2", "RedFruit3", "Skully1", "Skully2", "Skully3", "DrillArm1", "DrillArm2", "DrillArm3", "TimeBomb1", "TimeBomb2", "TimeBomb3", "Voltz1", "Voltz2", "Voltz3", "Lance", "Yo-Yo", "Wind", "Fan", "Boy'sBomb1", "Boy'sBomb2", "Boy'sBomb3", "Guard1", "Guard2", "Guard3", "CrackOut", "DoubleCrack", "TripleCrack", "Magnum", "MetaGel", "Snake", "CircleGun", "Mine", "RockCube", "Fanfare", "Discord", "Timpani", "Vdoll", "Asteroid1", "Asteroid2", "Asteroid3", "Recover10", "Recover30", "Recover50", "Recover80", "Recover120", "Recover150", "Recover200", "Recover300", "BusterUp", "PanelGrab", "AreaGrab", "GrabRevenge", "GrabBanish", "SlowGauge", "FastGauge", "PanelReturn", "Geddon1", "Geddon2", "Geddon3", "RainyDay", "ColorPoint", "ElementRage", "Blinder", "AirSpin1", "AirSpin2", "AirSpin3", "Invisible", "BubbleWrap", "Barrier", "Barrier100", "Barrier200", "NorthWind", "HolyPanel", "AntiFire", "AntiWater", "AntiElectric", "AntiWood", "AntiNavi", "AntiDamage", "AntiSword", "AntiRecovery", "CopyDamage", "Attack+10", "Navi+20", "FireHit1", "FireHit2", "FireHit3", "HotBody1", "HotBody2", "HotBody3", "AquaWhirl1", "AquaWhirl2", "AquaWhirl3", "SideBubble1", "SideBubble2", "SideBubble3", "ElecReel1", "ElecReel2", "ElecReel3", "CustomVolt1", "CustomVolt2", "CustomVolt3", "CurseShield1", "CurseShield2", "CurseShield3", "WavePit", "RedWave", "MudWave", "CactusBall1", "CactusBall2", "CactusBall3", "WoodyNose1", "WoodyNose2", "WoodyNose3", "", "", "", "", "", "", "DarkCircle", "DarkSword", "DarkInvis", "DarkPlus", "DarkLance", "DarkWide", "DarkThunder", "DarkRecovery", "DarkMeteor", "DarkDrill", "DarkTornado", "DarkSonic", "", "", "LifeAura", "Muramasa", "Guardian", "Anubis", "Attack+30", "BugFix", "DoublePoint", "Sanctuary", "FullCustom", "Meteors", "NumberBall", "Jealousy", "Poltergeist", "BlackWing", "Otenko", "JusticeOne", "NeoVariable", "ZSaber", "GunDelSolEX", "SuperVulcan", "Roll", "RollSP", "RollDS", "GyroMan", "GyroManSP", "GyroManDS", "NapalmMan", "NapalmManSP", "NapalmManDS", "SearchMan", "SearchManSP", "SearchManDS", "MagnetMan", "MagnetManSP", "MagnetManDS", "Meddy", "MeddyDS", "MeddySP", "ProtoMan", "ProtoManSP", "ProtoManDS", "NumberMan", "NumberManSP", "NumberManDS", "Colonel", "ColonelSP", "ColonelDS", "ShadowMan", "ShadowManSP", "ShadowManDS", "TomahawkMan", "TomahawkManSP", "TomahawkManDS", "KnightMan", "KnightManSP", "KnightManDS", "ToadMan", "ToadManSP", "ToadManDS", "ShadeMan", "ShadeManSP", "ShadeManDS", "BlizzardMan", "BlizzardManSP", "BlizzardManDS", "CloudMan", "CloudManSP", "CloudManDS", "CosmoMan", "CosmoManSP", "CosmoManDS", "LarkMan", "LarkManSP", "LarkManDS", "GridMan", "GridManSP", "GridManDS", "Django", "DjangoSP", "DjangoDS", "CannonMode", "CannonBall", "SwordMode", "Yo-YoMode", "DrillMode", "LCurseShield", "LStepSword", "LCounter", "ElementPower", "FinalGun", "", "", "", "", "", "", "", "", "", "", "Bass", "DeltaRay", "BugCurse", "MeteorKnuckle", "OmegaRocket", "BassAnomaly", "HolyDream", "BigHook", "CrossDivide", "BugCharge", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "MegaManNavi", "", "", "", "SearchManNavi", "", "", "ProtoManNavi", "NumberManNavi", "", "", "", "", "ShadowManNavi", "NapalmManNavi", "KnightManNavi", "ToadManNavi", "MagnetManNavi", "GyroManNavi", "ColonelNavi", "MeddyNavi", "TomahawkManNavi");

const blg_list = new Array("Cannon", "HiCannon", "MegaCannon", "AirShot", "Vulcan1", "Vulcan2", "Vulcan3", "SuperVulcan", "Spreader1", "Spreader2", "Spreader3", "TankCannon1", "TankCannon2", "TankCannon3", "GunDelSol1", "GunDelSol2", "GunDelSol3", "GunDelSolEX", "Yo-Yo", "FireBurner1", "FireBurner2", "FireBurner3", "WideShot", "TrainArrow1", "TrainArrow2", "TrainArrow3", "BubbleStar1", "BubbleStar2", "BubbleStar3", "Thunder", "DollThunder1", "DollThunder2", "DollThunder3", "ElecPulse1", "ElecPulse2", "ElecPulse3", "RiskyHoney1", "RiskyHoney2", "RiskyHoney3", "RollingLog1", "RollingLog2", "RollingLog3", "MachineGun1", "MachineGun2", "MachineGun3", "HeatDragon", "ElecDragon", "AquaDragon", "WoodDragon", "AirHockey", "DrillArm", "Tornado", "Static", "MiniBomb", "EnergyBomb", "MegaEnergyBomb", "FlashBomb1", "FlashBomb2", "FlashBomb3", "BlackBomb", "AquaNeedle1", "AquaNeedle2", "AquaNeedle3", "CornShot1", "CornShot2", "CornShot3", "BugBomb", "GrassSeed", "IceSeed", "PoisonSeed", "Sword", "WideSword", "LongSword", "WideBlade", "LongBlade", "FireSword", "AquaSword", "ElecSword", "BambooSword", "WindRacket", "StepSword", "VariableSword", "NeoVariable", "MoonBlade", "Muramasa", "MachineSword", "ElementSword", "AssassinSword", "CrackShot", "DoubleShot", "TripleShot", "WaveArm1", "WaveArm2", "WaveArm3", "AuraHead1", "AuraHead2", "AuraHead3", "LittleBoiler1", "LittleBoiler2", "LittleBoiler3", "SandWorm1", "SandWorm2", "SandWorm3", "AirRaid1", "AirRaid2", "AirRaid3", "FireHit1", "FireHit2", "FireHit3", "BurnSquare1", "BurnSquare2", "BurnSquare3", "Sensor1", "Sensor2", "Sensor3", "Boomer", "HiBoomer", "MegaBoomer", "Lance", "GolemHit1", "GolemHit2", "GolemHit3", "IronShell1", "IronShell2", "IronShell3", "AirSpin1", "AirSpin2", "AirSpin3", "Wind", "Fan", "Reflector1", "Reflector2", "Reflector3", "Snake", "SummonBlack1", "SummonBlack2", "SummonBlack3", "NumberBall", "Meteors", "JusticeOne", "Magnum", "CircleGun", "RockCube", "TimeBomb1", "Mine", "Fanfare", "Discord", "Timpani", "Silence", "Vdoll", "Guardian", "Anubis", "Otenko", "Recover10", "Recover30", "Recover50", "Recover80", "Recover120", "Recover150", "Recover200", "Recover300", "PanelGrab", "AreaGrab", "GrabBanish", "GrabRevenge", "PanelReturn", "Geddon", "HolyPanel", "Sanctuary", "ComingRoad", "GoingRoad", "SlowGauge", "FastGauge", "FullCustom", "BusterUp", "BugFix", "Invisible", "Barrier", "Barrier100", "Barrier200", "BubbleWrap", "LifeAura", "MagnetCoil", "WhiteCapsule", "Uninstall", "AntiNavi", "AntiDamage", "AntiSword", "AntiRecover", "CopyDamage", "LifeSync", "Attack+10", "Navi+20", "ColorPoint", "Attack+30", "DoublePoint", "ElementTrap", "ColonelArmy", "BlizzardBall", "TimeBomb2", "TimeBomb3", "BigBomb", "DarkTornado", "DarkCircle", "DarkMeteors", "DarkLance", "DarkWide", "", "", "", "", "", "", "", "", "", "", "", "", "", "Roll", "Roll2", "Roll3", "ProtoMan", "ProtoManEX", "ProtoManSP", "HeatMan", "HeatManEX", "HeatManSP", "ElecMan", "ElecManEX", "ElecManSP", "SlashMan", "SlashManEX", "SlashManSP", "EraseMan", "EraseManEX", "EraseManSP", "ChargeMan", "ChargeManEX", "ChargeManSP", "SpoutMan", "SpoutManEX", "SpoutManSP", "TomahawkMan", "TomahawkManEX", "TomahawkManSP", "TenguMan", "TenguManEX", "TenguManSP", "GroundMan", "GroundManEX", "GroundManSP", "DustMan", "DustManEX", "DustManSP", "BlastMan", "BlastManEX", "BlastManSP", "DiveMan", "DiveManEX", "DiveManSP", "CircusMan", "CircusManEX", "CircusManSP", "JudgeMan", "JudgeManEX", "JudgeManSP", "ElementMan", "ElementManEX", "ElementManSP", "Colonel", "ColonelEX", "ColonelSP", "Count", "CountEX", "CountSP", "Django", "Django2", "Django3", "PunchArm", "NeedleArm", "PuzzleArm", "BoomerArm", "SynchroTrigger", "DarkSword", "DarkThunder", "DarkRecover", "DarkInvisible", "DarkPlus", "", "", "", "", "", "", "", "", "", "", "Bass", "BigHook", "DeltaRay", "ColonelForce", "BugRiseSword", "BassAnomaly", "MeteorKnuckle", "CrossDivide", "HubBatch", "BugDeathThunder", "DoubleBeast", "Gregar", "Falzar", "", "", "", "", "", "", "", "MegaManV1", "MegaManV2", "MegaManV3", "MegaManV4", "MegaManV5", "MegaManV6", "MegaManV7", "MegaManV8", "MegaManV9", "MegaManV10", "MegaManV11", "MegaManV12", "MegaManV13", "MegaManV14", "MegaManSP", "HeatManV1", "HeatManV2", "HeatManV3", "HeatManV4", "HeatManV5", "HeatManV6", "HeatManV7", "HeatManV8", "HeatManV9", "HeatManV10", "HeatManV11", "HeatManV12", "HeatManV13", "HeatManV14", "HeatManSP", "ElecManV1", "ElecManV2", "ElecManV3", "ElecManV4", "ElecManV5", "ElecManV6", "ElecManV7", "ElecManV8", "ElecManV9", "ElecManV10", "ElecManV11", "ElecManV12", "ElecManV13", "ElecManV14", "ElecManSP", "SlashManV1", "SlashManV2", "SlashManV3", "SlashManV4", "SlashManV5", "SlashManV6", "SlashManV7", "SlashManV8", "SlashManV9", "SlashManV10", "SlashManV11", "SlashManV12", "SlashManV13", "SlashManV14", "SlashManSP", "EraseManV1", "EraseManV2", "EraseManV3", "EraseManV4", "EraseManV5", "EraseManV6", "EraseManV7", "EraseManV8", "EraseManV9", "EraseManV10", "EraseManV11", "EraseManV12", "EraseManV13", "EraseManV14", "EraseManSP", "ChargeManV1", "ChargeManV2", "ChargeManV3", "ChargeManV4", "ChargeManV5", "ChargeManV6", "ChargeManV7", "ChargeManV8", "ChargeManV9", "ChargeManV10", "ChargeManV11", "ChargeManV12", "ChargeManV13", "ChargeManV14", "ChargeManSP", "SpoutManV1", "SpoutManV2", "SpoutManV3", "SpoutManV4", "SpoutManV5", "SpoutManV6", "SpoutManV7", "SpoutManV8", "SpoutManV9", "SpoutManV10", "SpoutManV11", "SpoutManV12", "SpoutManV13", "SpoutManV14", "SpoutManSP", "TomahawkManV1", "TomahawkManV2", "TomahawkManV3", "TomahawkManV4", "TomahawkManV5", "TomahawkManV6", "TomahawkManV7", "TomahawkManV8", "TomahawkManV9", "TomahawkManV10", "TomahawkManV11", "TomahawkManV12", "TomahawkManV13", "TomahawkManV14", "TomahawkManSP", "TenguManV1", "TenguManV2", "TenguManV3", "TenguManV4", "TenguManV5", "TenguManV6", "TenguManV7", "TenguManV8", "TenguManV9", "TenguManV10", "TenguManV11", "TenguManV12", "TenguManV13", "TenguManV14", "TenguManSP", "GroundManV1", "GroundManV2", "GroundManV3", "GroundManV4", "GroundManV5", "GroundManV6", "GroundManV7", "GroundManV8", "GroundManV9", "GroundManV10", "GroundManV11", "GroundManV12", "GroundManV13", "GroundManV14", "GroundManSP", "DustManV1", "DustManV2", "DustManV3", "DustManV4", "DustManV5", "DustManV6", "DustManV7", "DustManV8", "DustManV9", "DustManV10", "DustManV11", "DustManV12", "DustManV13", "DustManV14", "DustManSP", "ProtoManV1", "ProtoManV2", "ProtoManV3", "ProtoManV4", "ProtoManV5", "ProtoManV6", "ProtoManV7", "ProtoManV8", "ProtoManV9", "ProtoManV10", "ProtoManV11", "ProtoManV12", "ProtoManV13", "ProtoManV14", "ProtoManSP");

let server_addr = "";
let server_ready = true;

let chip_data = new Uint8Array(4);

window.onload = (event) =>
{
	document.getElementById("netgate_type").selectedIndex = 0;
};

/****** Generates a list of chips for different Battle Chip Gates ******/
function generate_chip_list()
{
	let index = document.getElementById("netgate_type").selectedIndex;

	if(index == 0) { return; }

	document.getElementById("chip_list").innerHTML = "";
	let final_html = "";

	//Set chips for original Battle Chip Gate
	if(index == 1)
	{
		for(let x = 0; x < bcg_list.length; x++)
		{
			if(bcg_list[x].length > 0)
			{
				final_html += "<div class='chip_button' onclick=send_chip_data(" + (x + 1) + ")>" + bcg_list[x] + "</div>";
			}
		}
	}

	//Set chips for Progress Chip Gate
	else if(index == 2)
	{
		for(let x = 0; x < pcg_list.length; x++)
		{
			if(pcg_list[x].length > 0)
			{
				final_html += "<div class='chip_button' onclick=send_chip_data(" + (x + 1) + ")>" + pcg_list[x] + "</div>";
			}
		}
	}

	//Set chips for Beast Link Gate
	else
	{
		for(let x = 0; x < blg_list.length; x++)
		{
			if(blg_list[x].length > 0)
			{
				final_html += "<div class='chip_button' onclick=send_chip_data(" + (x + 1) + ")>" + blg_list[x] + "</div>";
			}
		}
	}

	document.getElementById("chip_list").innerHTML = final_html;
}

/****** Sends Battle Chip Data via POST ******/
function send_chip_data(chip_number)
{
	//Only send if the previous request has been confirmed as completed
	//This means the actual video stream may update at less than 5FPS
	//That depends on the server (GBE+), but that's fine, let the server do its thing
	if(server_ready)
	{
		chip_data[0] = chip_number >> 24;
		chip_data[1] = chip_number >> 16;
		chip_data[2] = chip_number >> 8;
		chip_data[3] = chip_number & 0xFF;

		server_ready = false;
		server_addr = "http://" + document.getElementById("netgate_address").value;

		fetch(server_addr, 
		{
			method: 'POST',
			body: chip_data,
		})

		.then(response =>
		{
			server_ready = true;
			document.getElementById("netgate_status").innerHTML = "Server Status : Good";
		})

		.catch((err) =>
		{
        		console.error('A fetch error occurred!');
			server_ready = true;
			document.getElementById("netgate_status").innerHTML.innerHTML = "Server Status : Comms Error!";
		});
	}
}

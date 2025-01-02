// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mpos_menu.h
// Date : July 28, 2019
// Description : Multi Plust On System menu
//
// Changes Pluster figurine

#include "mpos_menu.h"

#include "common/config.h"

/****** MPOS menu constructor ******/
mpos_menu::mpos_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Pluster type drop down menu
	QWidget* pluster_type_set = new QWidget;
	QLabel* pluster_type_label = new QLabel("Pluster Type : ");
 	
	pluster_type = new QComboBox;
	pluster_type->addItem("PF001 - Beetma");
	pluster_type->addItem("PF002 - Wyburst");
	pluster_type->addItem("PF003 - Gabrian");
	pluster_type->addItem("PF004 - Molly");
	pluster_type->addItem("PF005 - Hania");
	pluster_type->addItem("PF006 - Zagarian");
	pluster_type->addItem("PF007 - Tan Q");
	pluster_type->addItem("PF008 - Wariarm");
	pluster_type->addItem("PF009 - Doryuun");
	pluster_type->addItem("PF010 - Fezard");
	pluster_type->addItem("PF011 - Mashanta");
	pluster_type->addItem("PF012 - Gingardo");
	pluster_type->addItem("PF013 - Torastorm");
	pluster_type->addItem("PF014 - Gongoragon");
	pluster_type->addItem("PF015 - Mighty V");
	pluster_type->addItem("PF016 - Dorastorm");
	pluster_type->addItem("PF EX001 - Beetma EX");
	pluster_type->addItem("PF EX002 - Varouze");
	pluster_type->addItem("PF EX003 - Gigajoule");
	pluster_type->addItem("PF EX004 - Birdnick");
	pluster_type->addItem("PF EX005 - Poseihorn");
	pluster_type->addItem("PF EX006 - Terra");
	
	QHBoxLayout* pluster_type_layout = new QHBoxLayout;
	pluster_type_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	pluster_type_layout->addWidget(pluster_type_label);
	pluster_type_layout->addWidget(pluster_type);
	pluster_type_set->setLayout(pluster_type_layout);

	//Pluster figurine drop down menu - Set default for Beetma
	QWidget* pluster_figure_set = new QWidget;
	QLabel* pluster_figure_label = new QLabel("Pluster Figurine : ");

	pluster_figure = new QComboBox;

	QHBoxLayout* pluster_figure_layout = new QHBoxLayout;
	pluster_figure_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	pluster_figure_layout->addWidget(pluster_figure_label);
	pluster_figure_layout->addWidget(pluster_figure);
	pluster_figure_set->setLayout(pluster_figure_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(pluster_type_set);
	final_layout->addWidget(pluster_figure_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Multi Plust On System Configuration"));
	setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(pluster_type, SIGNAL(currentIndexChanged(int)), this, SLOT(update_pluster_type()));
	connect(pluster_figure, SIGNAL(currentIndexChanged(int)), this, SLOT(update_pluster_figure()));

	update_pluster_type();
	update_pluster_figure();
}

/****** Update Pluster figure type ******/
void mpos_menu::update_pluster_type()
{
	pluster_figure->clear();

	switch(pluster_type->currentIndex())
	{
		//Beetma
		case 0:
			pluster_figure->addItem("Beetma Advance");
			pluster_figure->addItem("Beetma Attack");
			pluster_figure->addItem("Beetma Guard");
			pluster_figure->addItem("Beetma Extra");
			pluster_figure->addItem("Beetma Power");
			pluster_figure->addItem("Beetma Rare #1");
			pluster_figure->addItem("Beetma Basic");
			pluster_figure->addItem("Beetma Rare #2");

			break;

		//Wyburst
		case 1:
			pluster_figure->addItem("Wyburst Advance");
			pluster_figure->addItem("Wyburst Attack");
			pluster_figure->addItem("Wyburst Guard");
			pluster_figure->addItem("Wyburst Extra");
			pluster_figure->addItem("Wyburst Power");
			pluster_figure->addItem("Wyburst Rare #1");
			pluster_figure->addItem("Wyburst Basic");
			pluster_figure->addItem("Wyburst Rare #2");

			break;

		//Gabrian
		case 2:
			pluster_figure->addItem("Gabrian Basic");
			pluster_figure->addItem("Gabrian Attack");
			pluster_figure->addItem("Gabrian Guard");
			pluster_figure->addItem("Gabrian Extra");
			pluster_figure->addItem("Gabrian Power");
			pluster_figure->addItem("Gabrian Rare #1");

			break;

		//Molly
		case 3:
			pluster_figure->addItem("Molly Basic");
			pluster_figure->addItem("Molly Attack");
			pluster_figure->addItem("Molly Guard");
			pluster_figure->addItem("Molly Extra");
			pluster_figure->addItem("Molly Power");
			pluster_figure->addItem("Molly Rare #1");

			break;

		//Hania
		case 4:
			pluster_figure->addItem("Hania Basic");
			pluster_figure->addItem("Hania Attack");
			pluster_figure->addItem("Hania Guard");
			pluster_figure->addItem("Hania Extra");
			pluster_figure->addItem("Hania Power");
			pluster_figure->addItem("Hania Rare #1");

			break;

		//Zagarian
		case 5:
			pluster_figure->addItem("Zagarian Basic");
			pluster_figure->addItem("Zagarian Attack");
			pluster_figure->addItem("Zagarian Guard");
			pluster_figure->addItem("Zagarian Extra");
			pluster_figure->addItem("Zagarian Power");
			pluster_figure->addItem("Zagarian Rare #1");

			break;

		//Tan Q
		case 6:
			pluster_figure->addItem("Tan Q Basic");
			pluster_figure->addItem("Tan Q Attack");
			pluster_figure->addItem("Tan Q Guard");
			pluster_figure->addItem("Tan Q Extra");
			pluster_figure->addItem("Tan Q Power");
			pluster_figure->addItem("Tan Q Rare #1");

			break;

		//Wariarm
		case 7:
			pluster_figure->addItem("Wariarm Basic");
			pluster_figure->addItem("Wariarm Attack");
			pluster_figure->addItem("Wariarm Guard");
			pluster_figure->addItem("Wariarm Extra");
			pluster_figure->addItem("Wariarm Power");
			pluster_figure->addItem("Wariarm Rare #1");

			break;

		//Doryuun
		case 8:
			pluster_figure->addItem("Doryuun Basic");
			pluster_figure->addItem("Doryuun Attack");
			pluster_figure->addItem("Doryuun Guard");
			pluster_figure->addItem("Doryuun Extra");
			pluster_figure->addItem("Doryuun Power");
			pluster_figure->addItem("Doryuun Rare #1");

			break;

		//Fezard
		case 9:
			pluster_figure->addItem("Fezard Basic");
			pluster_figure->addItem("Fezard Attack");
			pluster_figure->addItem("Fezard Guard");
			pluster_figure->addItem("Fezard Extra");
			pluster_figure->addItem("Fezard Power");
			pluster_figure->addItem("Fezard Rare #1");

			break;

		//Mashanta
		case 10:
			pluster_figure->addItem("Mashanta Basic");
			pluster_figure->addItem("Mashanta Attack");
			pluster_figure->addItem("Mashanta Guard");
			pluster_figure->addItem("Mashanta Extra");
			pluster_figure->addItem("Mashanta Power");
			pluster_figure->addItem("Mashanta Rare #1");

			break;

		//Gingardo
		case 11:
			pluster_figure->addItem("Gingardo Basic");
			pluster_figure->addItem("Gingardo Attack");
			pluster_figure->addItem("Gingardo Guard");
			pluster_figure->addItem("Gingardo Extra");
			pluster_figure->addItem("Gingardo Power");
			pluster_figure->addItem("Gingardo Rare #1");

			break;

		//Torastorm
		case 12:
			pluster_figure->addItem("Torastorm Basic");
			pluster_figure->addItem("Torastorm Attack");
			pluster_figure->addItem("Torastorm Guard");
			pluster_figure->addItem("Torastorm Extra");
			pluster_figure->addItem("Torastorm Power");
			pluster_figure->addItem("Torastorm Rare #1");

			break;

		//Gongoragon
		case 13:
			pluster_figure->addItem("Gongoragon Basic");
			pluster_figure->addItem("Gongoragon Attack");
			pluster_figure->addItem("Gongoragon Guard");
			pluster_figure->addItem("Gongoragon Extra");
			pluster_figure->addItem("Gongoragon Power");
			pluster_figure->addItem("Gongoragon Rare #1");

			break;

		//Mighty V
		case 14:
			pluster_figure->addItem("Mighty V Basic");
			pluster_figure->addItem("Mighty V Attack");
			pluster_figure->addItem("Mighty V Guard");
			pluster_figure->addItem("Mighty V Extra");
			pluster_figure->addItem("Mighty V Power");
			pluster_figure->addItem("Mighty V Rare #1");

			break;

		//Dorastorm
		case 15:
			pluster_figure->addItem("Dorastorm Basic");
			pluster_figure->addItem("Dorastorm Attack");
			pluster_figure->addItem("Dorastorm Guard");
			pluster_figure->addItem("Dorastorm Extra");
			pluster_figure->addItem("Dorastorm Power");
			pluster_figure->addItem("Dorastorm Rare #1");

			break;

		//Beetma EX
		case 16:
			pluster_figure->addItem("Beetma EX Basic");
			pluster_figure->addItem("Beetma EX Attack");
			pluster_figure->addItem("Beetma EX Guard");
			pluster_figure->addItem("Beetma EX Extra");
			pluster_figure->addItem("Beetma EX Power");
			pluster_figure->addItem("Beetma EX Rare #1");

			break;

		//Varouze
		case 17:
			pluster_figure->addItem("Varouze Basic");
			pluster_figure->addItem("Varouze Attack");
			pluster_figure->addItem("Varouze Guard");
			pluster_figure->addItem("Varouze Extra");
			pluster_figure->addItem("Varouze Power");
			pluster_figure->addItem("Varouze Rare #1");

			break;

		//Gigajoule
		case 18:
			pluster_figure->addItem("Gigajoule Basic");
			pluster_figure->addItem("Gigajoule Attack");
			pluster_figure->addItem("Gigajoule Guard");
			pluster_figure->addItem("Gigajoule Extra");
			pluster_figure->addItem("Gigajoule Power");
			pluster_figure->addItem("Gigajoule Rare #1");

			break;

		//Birdnick
		case 19:
			pluster_figure->addItem("Birdnick Basic");
			pluster_figure->addItem("Birdnick Attack");
			pluster_figure->addItem("Birdnick Guard");
			pluster_figure->addItem("Birdnick Extra");
			pluster_figure->addItem("Birdnick Power");
			pluster_figure->addItem("Birdnick Rare #1");

			break;

		//Poseihorn
		case 20:
			pluster_figure->addItem("Poseihorn Basic");
			pluster_figure->addItem("Poseihorn Attack");
			pluster_figure->addItem("Poseihorn Guard");
			pluster_figure->addItem("Poseihorn Extra");
			pluster_figure->addItem("Poseihorn Power");
			pluster_figure->addItem("Poseihorn Rare #1");

			break;

		//Tera
		case 21:
			pluster_figure->addItem("Tera Basic");
			pluster_figure->addItem("Tera Attack");
			pluster_figure->addItem("Tera Guard");
			pluster_figure->addItem("Tera Extra");
			pluster_figure->addItem("Tera Power");
			pluster_figure->addItem("Tera Rare #1");

			break;
			
	}
}

/****** Update Pluster figure ID ******/
void mpos_menu::update_pluster_figure()
{
	switch(pluster_type->currentIndex())
	{
		//Beetma
		case 0:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1780; break;
				case 1: config::mpos_id = 0x1740; break;
				case 2: config::mpos_id = 0x16C0; break;
				case 3: config::mpos_id = 0x1720; break;
				case 4: config::mpos_id = 0x1682; break;
				case 5: config::mpos_id = 0x1642; break;
				case 6: config::mpos_id = 0x1786; break;
				case 7: config::mpos_id = 0x1746; break;
			}

			break;

		//Wyburst
		case 1:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x16A0; break;
				case 1: config::mpos_id = 0x1660; break;
				case 2: config::mpos_id = 0x17E0; break;
				case 3: config::mpos_id = 0x1710; break;
				case 4: config::mpos_id = 0x17C2; break;
				case 5: config::mpos_id = 0x1622; break;
				case 6: config::mpos_id = 0x16C6; break;
				case 7: config::mpos_id = 0x1726; break;
			}

			break;

		//Gabrian
		case 2:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1690; break;
				case 1: config::mpos_id = 0x1650; break;
				case 2: config::mpos_id = 0x17D0; break;
				case 3: config::mpos_id = 0x1630; break;
				case 4: config::mpos_id = 0x17A2; break;
				case 5: config::mpos_id = 0x1762; break;
			}

			break;

		//Molly
		case 3:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1798; break;
				case 1: config::mpos_id = 0x1758; break;
				case 2: config::mpos_id = 0x16D8; break;
				case 3: config::mpos_id = 0x1738; break;
				case 4: config::mpos_id = 0x16B2; break;
				case 5: config::mpos_id = 0x1672; break;
			}

			break;

		//Hania
		case 4:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1688; break;
				case 1: config::mpos_id = 0x1648; break;
				case 2: config::mpos_id = 0x17C8; break;
				case 3: config::mpos_id = 0x1628; break;
				case 4: config::mpos_id = 0x1792; break;
				case 5: config::mpos_id = 0x1752; break;
			}

			break;

		//Zagarian
		case 5:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x17A4; break;
				case 1: config::mpos_id = 0x1764; break;
				case 2: config::mpos_id = 0x16E4; break;
				case 3: config::mpos_id = 0x1614; break;
				case 4: config::mpos_id = 0x16CA; break;
				case 5: config::mpos_id = 0x172A; break;
			}

			break;

		//Tan Q
		case 6:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1794; break;
				case 1: config::mpos_id = 0x1754; break;
				case 2: config::mpos_id = 0x16D4; break;
				case 3: config::mpos_id = 0x1734; break;
				case 4: config::mpos_id = 0x16AA; break;
				case 5: config::mpos_id = 0x166A; break;
			}

			break;

		//Wariarm
		case 7:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x17B0; break;
				case 1: config::mpos_id = 0x1770; break;
				case 2: config::mpos_id = 0x16F0; break;
				case 3: config::mpos_id = 0x1708; break;
				case 4: config::mpos_id = 0x16E2; break;
				case 5: config::mpos_id = 0x1612; break;
			}

			break;

		//Doryuun
		case 8:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x16B8; break;
				case 1: config::mpos_id = 0x1678; break;
				case 2: config::mpos_id = 0x17F8; break;
				case 3: config::mpos_id = 0x1704; break;
				case 4: config::mpos_id = 0x17F2; break;
				case 5: config::mpos_id = 0x160A; break;
			}

			break;

		//Fezard
		case 9:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x17A8; break;
				case 1: config::mpos_id = 0x1768; break;
				case 2: config::mpos_id = 0x16E8; break;
				case 3: config::mpos_id = 0x1618; break;
				case 4: config::mpos_id = 0x16D2; break;
				case 5: config::mpos_id = 0x1732; break;
			}

			break;

		//Mashanta
		case 10:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1684; break;
				case 1: config::mpos_id = 0x1644; break;
				case 2: config::mpos_id = 0x17C4; break;
				case 3: config::mpos_id = 0x1624; break;
				case 4: config::mpos_id = 0x178A; break;
				case 5: config::mpos_id = 0x174A; break;
			}

			break;

		//Gingardo
		case 11:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x16B4; break;
				case 1: config::mpos_id = 0x1674; break;
				case 2: config::mpos_id = 0x17F4; break;
				case 3: config::mpos_id = 0x160C; break;
				case 4: config::mpos_id = 0x17EA; break;
				case 5: config::mpos_id = 0x171A; break;
			}

			break;

		//Torastorm
		case 12:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x178C; break;
				case 1: config::mpos_id = 0x174C; break;
				case 2: config::mpos_id = 0x16CC; break;
				case 3: config::mpos_id = 0x172C; break;
				case 4: config::mpos_id = 0x169A; break;
				case 5: config::mpos_id = 0x165A; break;
			}

			break;

		//Gongoragon
		case 13:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x16AC; break;
				case 1: config::mpos_id = 0x166C; break;
				case 2: config::mpos_id = 0x17EC; break;
				case 3: config::mpos_id = 0x171C; break;
				case 4: config::mpos_id = 0x17DA; break;
				case 5: config::mpos_id = 0x163A; break;
			}

			break;

		//Mighty V
		case 14:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x169C; break;
				case 1: config::mpos_id = 0x165C; break;
				case 2: config::mpos_id = 0x17DC; break;
				case 3: config::mpos_id = 0x163C; break;
				case 4: config::mpos_id = 0x17BA; break;
				case 5: config::mpos_id = 0x177A; break;
			}

			break;

		//Dorastorm
		case 15:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x17BC; break;
				case 1: config::mpos_id = 0x177C; break;
				case 2: config::mpos_id = 0x16FC; break;
				case 3: config::mpos_id = 0x1702; break;
				case 4: config::mpos_id = 0x16FA; break;
				case 5: config::mpos_id = 0x1606; break;
			}

			break;

		//Beetma EX
		case 16:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1666; break;
				case 1: config::mpos_id = 0x17E6; break;
				case 2: config::mpos_id = 0x1716; break;
				case 3: config::mpos_id = 0x1696; break;
				case 4: config::mpos_id = 0x1656; break;
				case 5: config::mpos_id = 0x17D6; break;
			}

			break;

		//Varouze
		case 17:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1636; break;
				case 1: config::mpos_id = 0x17B6; break;
				case 2: config::mpos_id = 0x1776; break;
				case 3: config::mpos_id = 0x16F6; break;
				case 4: config::mpos_id = 0x170E; break;
				case 5: config::mpos_id = 0x168E; break;
			}

			break;

		//Gigajoule
		case 18:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x164E; break;
				case 1: config::mpos_id = 0x17CE; break;
				case 2: config::mpos_id = 0x162E; break;
				case 3: config::mpos_id = 0x17AE; break;
				case 4: config::mpos_id = 0x176E; break;
				case 5: config::mpos_id = 0x16EE; break;
			}

			break;

		//Birdnick
		case 19:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x161E; break;
				case 1: config::mpos_id = 0x179E; break;
				case 2: config::mpos_id = 0x175E; break;
				case 3: config::mpos_id = 0x16DE; break;
				case 4: config::mpos_id = 0x173E; break;
				case 5: config::mpos_id = 0x16BE; break;
			}

			break;

		//Poseihorn
		case 20:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x167E; break;
				case 1: config::mpos_id = 0x17FE; break;
				case 2: config::mpos_id = 0x1701; break;
				case 3: config::mpos_id = 0x1681; break;
				case 4: config::mpos_id = 0x1641; break;
				case 5: config::mpos_id = 0x17C1; break;
			}

			break;

		//Tera
		case 21:
			switch(pluster_figure->currentIndex())
			{
				case 0: config::mpos_id = 0x1621; break;
				case 1: config::mpos_id = 0x17A1; break;
				case 2: config::mpos_id = 0x1761; break;
				case 3: config::mpos_id = 0x16E1; break;
				case 4: config::mpos_id = 0x1611; break;
				case 5: config::mpos_id = 0x1791; break;
			}

			break;
	}
}

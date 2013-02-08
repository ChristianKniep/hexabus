/*
Config file for hexabus

Each app that is either optional or has config settings should have a section in this file.
There should be an ENABLE option that enables compilation and execution of the app, a DEBUG option which enables the DEBUG define of the app. You can add more options if your app is configurable somehow.
*/
#ifndef HEXABUS_CONFIG_H
#define HEXABUS_CONFIG_H

// button
#define BUTTON_PORT		PORTE
#define BUTTON_PIN		PINE
#define BUTTON_BIT		PE2

#define BUTTON_DEBOUNCE_MS			50
#define BUTTON_DOUBLE_CLICK_MS	500UL
#define	BUTTON_CLICK_MS					2000UL
#define	BUTTON_LONG_CLICK_MS		7000UL

#define BUTTON_DOUBLE_CLICK_ENABLED	0

#endif // HEXBAUS_CONFIG_H

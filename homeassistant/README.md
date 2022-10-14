## Home Assistant Examples

This folder contains some example automations on how you might use the TM1638 with Home Assistant.  While shown in YAML, the automations could also easily be created using the Home Assistant automation UI.

These are not meant as "blueprints" or even examples that you can just copy/paste into your own Home Assistant instance without modification.  For example, automations must be placed under an ```automation:``` or !included in some other manner.  These examples primarily just list the trigger and action sections and are meant to both show how the TM1638 can be might be used with Home Assistant.

File Name | Contents
----------|---------
button_automations.yaml| Examples of how to use button presses on the TM1638 in Home Assistant automations
date_time.yaml|Shows how to use your TM1638 as a clock or date display using Home Assistant's date and time sensors.
display_sensors.yaml|Provides some examples on how to publish your Home Assistant sensor values to the TM1638
use_leds.yaml|A couple of examples on how you might use the 8 independent LEDs on the TM1638 via Home Assistant automations

These are just a few simple examples... many more possibilities exist!
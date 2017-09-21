# SNAP-Automata

## Table of contents
1. [Introduction](#intro)
2. [Automation](#automata)
3. [Controller Configuration](#control)
4. [Home Assistant Additions](#hassadd)


## Introduction
I have always wanted a home automation system that is affordable and completely home-brew so I began on this journey in August 2017. This system was designed with cost minimization, energy efficiency, convenience, and ease of use for me and visitors in mind.

More detail is provided below, but a majority of this system is driven using WeMos D1 Pro ESP8266 devices, Arduino Uno devices, HomeAssistant and Mosquitto. This setup is completely free with the exception of the hardware. I will add details outlining the products I used in a future revision of this readme.

### Architecture

Server Infrastructure:
- Centos7 (minimal install)
- Provides the only access in or out of the IoT VLAN
- Remote syslogging to Splunk for diagnostics and historical device usage
- NTP from core switch
	
Above server virtualized using ESXI (Minimal specs due to nature of device)
- i7 Processor
- Gigabit ethernet connnection
- 2GB RAM
- 15GB disk space

Network Infrastructure:
- IoT VLAN isolates all devices, available via wireless and wired networks
- Firewall configured to block all inbound, outbound, and cross-device communication 
- Only communication allowed directly through MQTT broker
	
Software Infrastructure:
- MQTT Dash for Android
- IoT endpoints using MQTT library for control and 
- HomeAssistant as automation and interface
- Mosquitto as MWTT broker	
	
	
## Automation
For the time being only lighting has been implemented but I eventually plan to incorporate mechanical devices and sensors.

Rules:

- Front path lights turn on before leaving for work and off at sunrise
- Front path lights turn on again at sundown and turn off at a set time
- Side path lights, pond lights, and rear porch lights only turn on when manually triggered
- All exterior lights turn off at midnight regardless of how they were triggered
- First floor side lamp follows schedule of front path lights
- Display cases are triggered manually and will stay lit for two hours. I will eventually turn these into presence devices

## Controller Configuration
### Home Assistant
The minimal install of Centos doesn't come with the packages we need to run HomeAssistant, so our first step will be getting those installed.

```
sudo yum install centos-release-scl
sudo yum install rh-python35
```

Set up a system user and group for home assistant to use. I will configure it to run under the user and group 'homeassistant'. I HIGHLY recommend not running software as root for security and granularity reasons.

```
sudo useradd --system homeassistant
sudo groupadd homeassistant
sudo usermod -a -G homeassistant
```

Next lets create the install dir, then home directory

```
sudo mkdir /srv/homeassistant
sudo chown homeassistant:homeassistant /srv/homeassistant
sudo su -s /bin/bash homeassistant
```

Next enable the software collection installed above and verify the Python version

```
scl enable rh-python35 bash
python --version
```

It should return `Python 3.5.x`. I decided to install and use this in a virtual environment. You can decide what is best for you with some basic research.

```
virtualenv -p python3 /srv/homeassistant
source /srv/homeassistant/bin/activate
```

Your prompt should now include (homeassistant). Now we are ready to install HASS in our virtual environment.

```
pip3 install --upgrade homeassistant
```

Home assistant should now be installed. It can by using

```
sudo -u homeassistant -H /srv/homeassistant/bin/hass
```

In order to make Home Assistant run on boot, we need to create a service. Luckily this already exists! Just download the service file from my repo above, or run the following command:

```
sudo wget https://raw.githubusercontent.com/BenchPressesBooks/SNAP-Automata/master/services/home-assistant%40homeassistant.service -O /etc/systemd/system/home-assistant@homeassistant.service
```

To make the daemon aware of the new configuration, reload systemd

```
sudo systemctl --system daemon-reload
sudo systemctl enable home-assistant@homeassistant
sudo systemctl start home-assistant@homeassistant
```

Upgrading HASS is easy now, just type

```
sudo su -s /bin/bash homeassistant
source /srv/homeassistant/bin/activate
pip3 install --upgrade homeassistant
```

The main configuration file for HomeAssistant is at `/home/hass/.homeassistant/configuration.yaml`
The main logs file is at `/home/hass/.homeassistant/home-assistant.log`


### Mosquitto MQTT Messaging Broker
To install Mosquitto you will have to add another package to centos that is not included in the minimum install

```
sudo yum -y install epel-release
```

Now you can install the broker using yum. This is preferred to a manual install as `yum update` will keep this package at the latest version.

```
sudo yum install mosquitto
```

You will probably also want this to run at startup. Luckily the service daemon is already included with the package

```
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
```

MQTT is a great protocol because it allows for ACL based security, which means you will need to create users

```
sudo touch /etc/mosquitto/conf.d/pwfile
sudo mosquitto_passwd /etc/mosquitto/conf.d/pwfile <mosquitto_username> <mosquitto_password>
```

To configure publishing and subscribing, create an `aclfile`

```
sudo touch /etc/mosquitto/conf.d/aclfile
```

NOTE: Using the batch mode above for mosquitto_passwd will make the password available in your bashhistory file. Heed caution when using this method. I used `ha` as my main user for HomeAssistnat to communicate with Mosquitto. This will be visible in my ACLFILE in the repo above.
To link Home Assistant with the Mosquitto broker, the `configuration.yaml` will need to have the lines below added

```yaml
mqtt:
  broker: 'localhost' #127.0.0.1
  port: 1883
  client_id: 'ha'
  username: '<mosquitto_username>'
  password: '<mosquitto_password>'
```

The configuration file for Mosquitto is located at `/etc/mosquitto/conf.d/mosquitto.conf`
The log file is located at `/var/log/mosquitto/mosquitto.log`


## Home Assistant Additions
To start with this project, I decided to include DarkSky as a weather monitor. You will have to register at their website to get an API key. Once you receive that the sensor can be added to the sensors.yaml file. You will find mine in the configuration directory of the repo. I may eventually add more sensors to this project, but for the time being I am focusing on hardware design and cost-cutting.
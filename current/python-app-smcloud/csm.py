from __future__ import with_statement
import sqlite3
from contextlib import closing
from flask import Flask, redirect, request, g, url_for, abort, render_template, jsonify


#########################
# configuration
#########################
DEBUG = True
SECRET_KEY = 'devkey'

app = Flask(__name__)
app.config.from_object(__name__)
app.config.from_envvar('FLASKR_SETTINGS', silent=True)


#########################
# app - web side
#########################

# regions
regions = dict()

# status codes
CR_ERROR = 13
CR_OKAY = 12


@app.route('/')
def show_regions():
	return render_template('show_regions.html', regions=regions)


@app.route('/reset/')
def reset():
	# clear region and csm data
	global regions
	regions = dict()
	return redirect(url_for('show_regions'))

#########################
# app - phone side
#########################

@app.route('/readparking/<int:x>/<int:y>/<int:id>/<int:modified_time>/')
def read_parking(x, y, id, modified_time):
	"""Respond to a parking spot read from a node"""
	# create region entry if not found
	if not (x,y) in regions.keys():
		regions[(x,y)] = {'spots': 2L}
	
	# read parking spot
	response = regions[(x,y)]
	response['status'] = CR_OKAY
	return jsonify(response)

	
@app.route('/requestparking/<int:x>/<int:y>/<int:id>/<int:modified_time>/')
def request_parking(x, y, id, modified_time):
	"""Respond to a parking spot request from a node"""
	# create region entry if not found
	if not (x,y) in regions.keys():
		regions[(x,y)] = {'spots': 2L}
		
	# if there are spots available, success
	if regions[(x,y)]['spots'] > 0L:
		regions[(x,y)]['spots'] = regions[(x,y)]['spots'] - 1
		response = regions[(x,y)]
		response['status'] = CR_OKAY
	else: # failure
		response = regions[(x,y)]
		response['status'] = CR_ERROR
	
	return jsonify(response)
	
	
@app.route('/releaseparking/<int:x>/<int:y>/<int:id>/<int:modified_time>/')
def release_parking(x, y, id, modified_time):
	"""Respond to a parking spot release from a node"""
	
	# if region does not exist alrady, error
	if not (x,y) in regions.keys():
		response = {'status': CR_ERROR}
	else: # release parking spot
		regions[(x,y)]['spots'] = regions[(x,y)]['spots'] + 1
		response = regions[(x,y)]
		response['status'] = CR_OKAY
	
	return jsonify(response)
	

#########################
# run
#########################
if __name__ == '__main__':
	app.run(host='0.0.0.0', port=4212)

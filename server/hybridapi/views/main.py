#
# Copyright (C) 2023 onwards LabsLand, Inc.
# All rights reserved.
#
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.
#

import os
import time
from logging import getLogger
from typing import Optional
from functools import wraps

from flask import Blueprint, g, request, jsonify, current_app, render_template

logger = getLogger(__name__)

main_blueprint = Blueprint('main', __name__)

@main_blueprint.route('/')
def index():
    return render_template("index.html", sep=os.sep)

def supports_long_polling(func):
    """
    Initialize a view to support long polling
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        g.wait_for_response = request.args.get('wait_for_response')
        if g.wait_for_response:
            try:
                g.wait_for_response = float(g.wait_for_response)
            except:
                g.wait_for_response = None

        g.previous_response = request.args.get('previous_response')
        if g.previous_response is None:
            g.wait_for_response = None
        else:
            if g.wait_for_response is None:
                g.wait_for_response = 10
        g.long_polling_initial_time = time.time()
        return func(*args, **kwargs)

    return wrapper

def check_long_polling(value) -> bool:
    """
    If long polling is requested and the value is the same, wait for the required time
    and return True. Otherwise, return False
    """
    elapsed = time.time() - g.long_polling_initial_time
    if g.wait_for_response is not None:
        if elapsed < g.wait_for_response and g.previous_response == value:
            time.sleep(0.1)
            return True
    return False


@main_blueprint.route('/api')
def api_index():
    return "API root"

@main_blueprint.route('/api/gpios/sim2dut/', defaults={'number': None})
@main_blueprint.route('/api/gpios/sim2dut/<int:number>')
@supports_long_polling
def gpios_sim2dut(number: Optional[int]):
    """
    Obtain the data that the simulation tries to send to the web.

    Do this by reading a file that the simulation pushes.

    There are two options: obtaining the full string (i.e., all the gpios at once) or
    obtaining only one.

    Also, you can provide: ?wait_for_response=15&previous_response=101

    if you want the server to wait at most 15 seconds OR whenever the new response != 101.
    """
    sim2dut_gpios_size = current_app.config['SIMULATION_CONFIG']['gpios']['sim2dut']['number']
    if number is not None:
        if number < 0 or number >= sim2dut_gpios_size:
            return jsonify(success=False, message=f"Requested GPIO number {number} is outside the range of sim2dut: 0..{sim2dut_gpios_size-1}"), 400

    binary_dir = current_app.config['BINARY_DIRECTORY']
    sim2dut_gpios = os.path.join(binary_dir, current_app.config['SIMULATION_CONFIG']['gpios']['sim2dut']['file'])

    while True:
        if not os.path.exists(sim2dut_gpios):
            logger.warning(f"File {sim2dut_gpios} not found. It might be normal if file is starting.")
            if number is None:
                value = '0' * sim2dut_gpios_size
            else:
                value = '0'
        else:
            content = open(sim2dut_gpios, 'r').read().strip()
            content = content[:sim2dut_gpios_size]
            content = content + '0' * (sim2dut_gpios_size - len(content))
            if number is None:
                value = content
            else:
                value = content[number]

        if check_long_polling(value):
            continue

        if number is None:
            return jsonify(success=True, value=value)
        else:
            return jsonify(success=True, value=value == '1')


@main_blueprint.route('/api/gpios/dut2sim/', defaults={'number': None})
@main_blueprint.route('/api/gpios/dut2sim/<int:number>', methods=['GET', 'POST'])
@supports_long_polling
def gpios_dut2sim(number: Optional[int]):
    binary_dir = current_app.config['BINARY_DIRECTORY']
    dut2sims_gpios = os.path.join(binary_dir, current_app.config['SIMULATION_CONFIG']['gpios']['dut2sim']['file'])
    dut2sims_gpios_size = current_app.config['SIMULATION_CONFIG']['gpios']['dut2sim']['number']

    if number is not None:
        if number < 0 or number >= dut2sims_gpios_size:
            return jsonify(success=False, message=f"Requested GPIO number {number} is outside the range of dut2simulations: 0..{dut2sims_gpios_size-1}"), 400

    if os.path.exists(dut2sims_gpios):
        content = open(dut2sims_gpios).read().strip()
        content = content[:dut2sims_gpios_size]
        content = content + '0' * (dut2sims_gpios_size - len(content))
    else:
        content = '0' * dut2sims_gpios_size
        logger.warning(f"File {dut2sims_gpios} not found. It might be normal if sim is starting.")

    if request.method == 'GET':
        while True:
            if os.path.exists(dut2sims_gpios):
                content = open(dut2sims_gpios).read().strip()
                content = content[:dut2sims_gpios_size]
                content = content + '0' * (dut2sims_gpios_size - len(content))
            else:
                content = '0' * dut2sims_gpios_size

            if number is None:
                value = content
            else:
                value = content[number]

            if check_long_polling(value):
                continue

            if number is None:
                return jsonify(success=True, value=value)
            else:
                return jsonify(success=True, value=value == '1')

    # If POST
    if number is None:
        return jsonify(success=False, message="Cannot write in batch (as for now)"), 400

    request_data = request.get_json(silent=True, force=True) or {}
    value = request_data.get('value')
    if value is None:
        return jsonify(success=False, message="Invalid 'value' provided in json"), 400

    content_as_list = list(content)
    content_as_list[number] = '1' if value in (True, 1, '1', 'true', 'True', 'TRUE') else '0'
    content = ''.join(content_as_list)

    if os.path.exists(dut2sims_gpios):
        mode = 'r+' # Be able to go to the beginning and change it
    else:
        mode = 'w' # File does not exist, add the content

    with open(dut2sims_gpios, mode) as dut2sims_write_mode:
        dut2sims_write_mode.seek(0)
        dut2sims_write_mode.write(content)
        dut2sims_write_mode.truncate(dut2sims_gpios_size)
        dut2sims_write_mode.flush()

    return jsonify(success=True)

@main_blueprint.route('/api/messages/sim2web/')
@supports_long_polling
def messages_sim2web():
    binary_dir = current_app.config['BINARY_DIRECTORY']
    sim2web_messages = os.path.join(binary_dir, current_app.config['SIMULATION_CONFIG']['messages']['sim2web']['file'])
    while True:
        if os.path.exists(sim2web_messages):
            value = open(sim2web_messages).read().strip()
        else:
            value = ''

        if check_long_polling(value):
            continue

        return jsonify(success=True, value=value)


@main_blueprint.route('/api/messages/web2sim/', methods=['GET', 'POST'])
@supports_long_polling
def messages_web2sim():
    binary_dir = current_app.config['BINARY_DIRECTORY']
    web2sim_messages = os.path.join(binary_dir, current_app.config['SIMULATION_CONFIG']['messages']['web2sim']['file'])
    if request.method == 'GET':
        while True:
            if os.path.exists(web2sim_messages):
                value = open(web2sim_messages).read()
            else:
                value = ""

            if check_long_polling(value):
                continue

            return jsonify(success=True, value=value)

    if os.path.exists(web2sim_messages):
        mode = 'r+'
    else:
        mode = 'w'

    request_data = request.get_json(silent=True, force=True) or {}
    value = request_data.get('value')
    if value is None:
        return jsonify(success=False, message="Invalid 'value' provided in json"), 400

    with open(web2sim_messages, mode) as web2sim_messages_file:
        web2sim_messages_file.seek(0)
        web2sim_messages_file.write(value)
        web2sim_messages_file.truncate(len(value))
        web2sim_messages_file.flush()

    return jsonify(success=True)

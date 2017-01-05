# -*- coding: utf-8 -*-

import csv

STATE_INITIAL = 1
STATE_MOVING = 2

# Ignore lower-bound of move points
LEAST_MOVE_STEP = 3

# If the peak value of move point set less than this value, ignore it!
LEAST_MOVE_PEAK = 0.1

"""
@brief  Calculate the disatance of moving according to its acc. value
"""
def calculate_distance(acc_points):
    v = 0.0
    t = 1
    distance = 0.0
    for a in acc_points:
        # Calculate the distance
        s = v * t + 0.5 * a * t * t;
        distance = distance + s
        # Calculate the next velocity
        v = v + a * t;

    return distance



f = open('test_data_01.csv', 'r')

state = STATE_INITIAL
for row in csv.DictReader(f):
    if state == STATE_INITIAL:
        state = STATE_MOVING
        prev_value = 0.0
        curr_value = float(row['y1_cali'])
        move_points = []
        begin_of_move = row['No']
        continue

    prev_value = curr_value
    curr_value = float(row['y1_cali'])

    if prev_value < 0.0 and curr_value > 0.0:
        if len(move_points) > LEAST_MOVE_STEP and max(move_points) > LEAST_MOVE_PEAK:
            print("Move from {} -> {}, distance: {}".format(begin_of_move, (int(row['No']) - 1), calculate_distance(move_points)))

        move_points = []
        begin_of_move = row['No']

    move_points.append(curr_value)

f.close()

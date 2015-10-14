#
# This file is part of bgpstream
#
# CAIDA, UC San Diego
# bgpstream-info@caida.org
#
# Copyright (C) 2015 The Regents of the University of California.
# Authors: Alistair King, Chiara Orsini
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#


 #!/usr/bin/env python

import sqlite3
import argparse


def create_tables(db_conn):
    c = db_conn.cursor()    
    c.execute('''SELECT name FROM sqlite_master WHERE type='table' AND name='bgp_data' ''')
    if len(c.fetchall()) == 0:
        c.execute('''CREATE TABLE bgp_data
                 (collector_id integer,
                 type_id integer,
                 file_time timestamp,
                 file_path text,
                 ts timestamp default (strftime('%s', 'now')),
                 PRIMARY KEY(collector_id, type_id, file_time))''')
    c.execute('''SELECT name FROM sqlite_master WHERE type='table' AND name='collectors' ''')
    if len(c.fetchall()) == 0:
        c.execute('''CREATE TABLE collectors
                 (id integer PRIMARY KEY,
                 project text,
                 name text)''')
    c.execute('''SELECT name FROM sqlite_master WHERE type='table' AND name='bgp_types' ''')
    if len(c.fetchall()) == 0:
        c.execute('''CREATE TABLE bgp_types
                 (id integer PRIMARY KEY,
                 name text)''')
        c.execute("INSERT INTO bgp_types VALUES ('1','ribs')")
        c.execute("INSERT INTO bgp_types VALUES ('2','updates')")
    c.execute('''SELECT name FROM sqlite_master WHERE type='table' AND name='time_span' ''')
    if len(c.fetchall()) == 0:
        c.execute('''CREATE TABLE time_span
                 (collector_id integer,
                 bgp_type_id integer,
                 time_span integer,
                 PRIMARY KEY(collector_id, bgp_type_id))''')
    db_conn.commit()


def add_new_bgp_data(db_conn, mrt_file, project, collector, bgp_type, file_time, updates_time_span):
    c = db_conn.cursor()
    col_id = 0
    # get the collector id (or create a new one if it doesn't exist)
    c.execute('''SELECT id FROM collectors WHERE project=? AND name=?''',
              [project, collector])
    res = c.fetchone()
    if res is None:
        c.execute('''SELECT count(*) FROM collectors''')
        col_id = c.fetchone()[0] + 1
        c.execute('''INSERT INTO collectors VALUES(?,?,?)''', [col_id, project, collector])
        # time span for ribs is constant and set to 120
        c.execute('''INSERT OR REPLACE INTO time_span VALUES(?,1,120)''',[col_id])
    else:
        col_id = res[0]
    # Set or update the time span information
    c.execute('''SELECT time_span.time_span FROM time_span WHERE collector_id = ? AND bgp_type_id = 2''', [col_id])
    res = c.fetchone()
    # the policy used is that the largest update time span wins
    if res is None or res[0] < updates_time_span:
        c.execute('''INSERT OR REPLACE INTO time_span VALUES(?,2,?)''',
        [col_id, updates_time_span])
    # check/get type id
    c.execute('''SELECT id FROM bgp_types WHERE name=? ''', [bgp_type])
    res = c.fetchone()
    if not res[0]:
        print "bgp type " + bgp_type + " not supported!"
        db_conn.commit()
        return 0
    else:
        type_id = res[0]
    # insert or replace bgp_data information
    c.execute('''INSERT OR REPLACE INTO bgp_data
             (collector_id, type_id, file_time, file_path)
             VALUES(?,?,?,?)''', [col_id, type_id, file_time, mrt_file])
    db_conn.commit()
    return 1



def list_all_files(db_conn):
    c = db_conn.cursor()
    c.execute('''SELECT collectors.project, collectors.name,
                        bgp_types.name, time_span.time_span,
                        bgp_data.file_time, bgp_data.file_path, bgp_data.ts
              FROM  collectors JOIN bgp_data JOIN bgp_types JOIN time_span
              WHERE bgp_data.collector_id = collectors.id  AND
                    bgp_data.collector_id = time_span.collector_id AND
                    bgp_data.type_id = bgp_types.id AND
                    bgp_data.type_id = time_span.bgp_type_id
                    ''')
    res = c.fetchall()
    print "Files in database: " + str(len(res))
    for line in res:
        print line    



parser = argparse.ArgumentParser()
parser.add_argument("sqlite_db", help="file containing the sqlite database",
                   type=str)
parser.add_argument("-l","--list_files", help="list the mrt files in the database",
                     action="store_true")
parser.add_argument("-M","--add_mrt_file", help="path to the mrt file to add to the database",
                     default=None, action="store",type=str)
parser.add_argument("-p","--proj", help="bgp project",
                    default=None, action='store',type=str)
parser.add_argument("-c","--coll", help="bgp collector",
                    default=None, action='store',type=str)
parser.add_argument("-t","--bgp_type", help="bgp type",
                    default=None, action='store',type=str)
parser.add_argument("-T","--file_time", help="time associated with the mrt file",
                    default=-1, action='store',type=int)
parser.add_argument("-u","--updates_time_span", help="updates time span",
                    default=-1, action='store',type=int)
args = parser.parse_args()

# connect to the database
conn = sqlite3.connect(args.sqlite_db)

# create tables (if they do not exist)
create_tables(conn)

if not args.list_files and not args.add_mrt_file:
    print "No actions required, creating the database file " + args.sqlite_db

if args.list_files:
    # output the list of files
    list_all_files(conn)


# Add a new mrt file to the database
if args.add_mrt_file:
    action_result = 0
    if args.proj and args.coll and args.bgp_type and args.file_time:
        if args.bgp_type == "updates" and args.updates_time_span == -1:
            print "Could not add mrt file: please provide time span for the current update file"
        else:            
            res = add_new_bgp_data(conn, args.add_mrt_file, args.proj, args.coll,
                                   args.bgp_type, args.file_time, args.updates_time_span)
            if res != 1:
                "Could not add mrt file: wrong parameters" 
    else:
        print "Could not add mrt file: please provide project, collector, bgp type, and file time"

# # We can also close the connection if we are done with it.
# # Just be sure any changes have been committed or they will be lost.
conn.close()


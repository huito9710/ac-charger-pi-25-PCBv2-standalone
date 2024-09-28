import sqlite3
from datetime import datetime

class Db():
    def __init__(self, dbFileName):
        self.conn = sqlite3.connect(dbFileName)
        self.authStatusTbl = {
            1: "Accepted",
            2: "Blocked",
            3: "Expired",
            4: "Invalid",
            5: "ConcurrentTx"
        }

    def getListVersion(self):
        cur = self.conn.cursor();
        cur.execute("select list_ver, list_last_update from list_info where rowid = (SELECT MAX(rowid) FROM list_info)")
        rows = cur.fetchall()
        nrows = len(rows)
        verInfo = ()
        if nrows > 0:
            print("no. results: {}".format(nrows))
            r = rows[nrows-1]
            dt = datetime.fromtimestamp(r[1])
            print("list_version: {} @ {}".format(r[0], dt.isoformat()))
            verInfo = (r[0], dt)
        cur.close()            
        return verInfo

    def getTagList(self,beginWith=None):
        cur = self.conn.cursor();
        if beginWith is None:
            queryStr = "select * from auth_status"
        else:
            queryStr = "select * from auth_status where tag_id like '" + beginWith + "%'"
            print(queryStr)
        cur.execute(queryStr)
        tags = []
        r = cur.fetchone()
        while r != None:
            self.print_row(r)
            authStatus = self.authStatusTbl.get(r[1], "Unknown");
            try:
                expiryDate = datetime.fromtimestamp(r[2])
                tags.append((r[0], authStatus, expiryDate))
            except:
                tags.append((r[0], authStatus))
            r = cur.fetchone()
        cur.close()
        return tags
        
    def print_row(self, r):
        authStatus = self.authStatusTbl.get(r[1], "Unknown");
        try:
            expiryDate = datetime.fromtimestamp(r[2])
            print("Tag: {} {} {}".format(r[0], authStatus, expiryDate.isoformat()))
        except:
            print("Tag: {} {}".format(r[0], authStatus))
        



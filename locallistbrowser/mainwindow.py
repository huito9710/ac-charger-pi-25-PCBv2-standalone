import tkinter as tk
import tkinter.font as tkfont
from tkinter.filedialog import askopenfilename
import locallistdb as locallist

class Application(tk.Frame):
    #dbFile = "C:/Users/Frankie.F/OneDrive/Documents/Charger.native_win64/Databases/ocpp_local_auth_list.db3"
    dbFile = "/home/pi/upfiles/ocpp_local_auth_list.db3"
    
    def __init__(self, master=None):
        tk.Frame.__init__(self, master)

        self.db = None
        self.searchVar = tk.StringVar()
        
        self.master.title("LocalList DB Browser")
        self.master.maxsize(800,480)
        #self.master.state('zoomed')
        self.master.geometry("800x480")
        self.pack()
        self.createWidgets()
        

    def createWidgets(self):
        topBar = tk.Frame(self);
        topBar.pack(side="top", expand=1, fill='both')
        
        self.dbfileButton = tk.Button(topBar)
        self.dbfileButton["text"] = "Open DB File";
        self.dbfileButton["command"] = self.dbfileButtonClicked
        self.dbfileButton.pack(side="left", expand=0, padx=10, pady=5)

        self.listVersionLabel = tk.Label(topBar)
        self.listVersionLabel.pack(side="left", expand=0, padx=10, pady=5)
        
        self.QUIT = tk.Button(topBar, text="QUIT", fg="red", command=self.master.destroy)
        self.QUIT.pack(side="right", padx=10, pady=5)

        searchBar = tk.Frame(self);
        searchBar.pack(side="top", expand=1, fill='both')
        self.searchBox = tk.Entry(searchBar, textvariable=self.searchVar)
        self.searchBox.pack(side="left", expand=1, fill='both', padx=10, pady=5)
        self.searchButton = tk.Button(searchBar)
        self.searchButton["text"] = "Search"
        self.searchButton["command"] = self.searchButtonClicked
        self.searchButton["state"] = "disabled"
        self.searchButton.pack(side="right", expand=1, fill='both', padx=10, pady=5)
        
        self.tagListBox = tk.Listbox(self.master)
        self.tagListBox["font"] = tkfont.Font(family="Courier", size=14)
        self.tagListBox.pack(side="bottom", padx=10, pady=5, expand=1, fill='both')

    def updateTagListBox(self, tags):
        self.tagListBox.delete(0, tk.END)
        if tags is not None:
            for t in tags:
                if len(t) > 2:
                    line = t[0] + " " + t[1] + " " + t[2].isoformat()
                else:
                    line = t[0] + " " + t[1]
                self.tagListBox.insert(tk.END, line)
                
    def dbfileButtonClicked(self):
        # Just open the file for now, File Dialog is a bit messy with Python 3.5.3.
        if self.db is None:
            try:
                self.db = locallist.Db(Application.dbFile)
            except:
                askedDbFile = askopenfilename()
                self.db = locallist.Db(askedDbFile)
        # display list version
        (listVer, verDate) = self.db.getListVersion()
        self.listVersionLabel["text"] = "Version: " + str(listVer)  + " Updated: " + verDate.isoformat()
        # display items
        tags = self.db.getTagList()
        self.updateTagListBox(tags)
        # allow search
        self.searchButton["state"] = "normal"
        
        
    def searchButtonClicked(self):
        stxt = self.searchVar.get()
        print("Search for {}".format(stxt))
        tags = None
        if len(stxt) > 0:
            tags = self.db.getTagList(stxt)
        else:
            # show all
            tags = self.db.getTagList()
        self.updateTagListBox(tags)
        
        
        

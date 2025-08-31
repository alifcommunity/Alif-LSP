"""
Simple UI Test for Alif LSP Server
"""
import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
import os
import json
from lsp_client import LSPClient


class LSPTestUI:
    def __init__(self, root):
        self.root = root
        self.root.title("اختبار خادم ألف LSP - Alif LSP Server Test")
        self.root.geometry("800x600")
        
        self.client: LSPClient = None
        self.server_path = ""
        self.current_document_uri = ""
        
        self.setup_ui()
        
    def setup_ui(self):
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        
        # Server path selection
        ttk.Label(main_frame, text="Server Path:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.server_path_var = tk.StringVar(value="../windows-build/x64/Release/alsp.exe")
        server_entry = ttk.Entry(main_frame, textvariable=self.server_path_var, width=50)
        server_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), pady=5, padx=5)
        ttk.Button(main_frame, text="Browse", command=self.browse_server, width=12).grid(row=0, column=2, pady=5, padx=5, ipadx=3, ipady=3)
        
        # Server control buttons
        button_frame = ttk.Frame(main_frame)
        button_frame.grid(row=1, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))
        
        self.start_btn = ttk.Button(button_frame, text="Start Server", command=self.start_server, width=15)
        self.start_btn.pack(side=tk.LEFT, padx=5, pady=3, ipadx=5, ipady=5)
        
        self.stop_btn = ttk.Button(button_frame, text="Stop Server", command=self.stop_server, state="disabled", width=15)
        self.stop_btn.pack(side=tk.LEFT, padx=5, pady=3, ipadx=5, ipady=5)
        
        self.init_btn = ttk.Button(button_frame, text="Initialize", command=self.initialize_server, state="disabled", width=15)
        self.init_btn.pack(side=tk.LEFT, padx=5, pady=3, ipadx=5, ipady=5)
        
        # Status label
        self.status_var = tk.StringVar(value="Server not started")
        ttk.Label(main_frame, textvariable=self.status_var, foreground="red").grid(row=2, column=0, columnspan=3, pady=5)
        
        # Document section
        doc_frame = ttk.LabelFrame(main_frame, text="Document", padding="5")
        doc_frame.grid(row=3, column=0, columnspan=3, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        doc_frame.columnconfigure(1, weight=1)
        # Don't make any rows expandable to prevent text area from covering buttons
        
        # Document URI
        ttk.Label(doc_frame, text="URI:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.uri_var = tk.StringVar(value="file:///test.alif")
        uri_entry = ttk.Entry(doc_frame, textvariable=self.uri_var, width=50)
        uri_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), pady=2, padx=5)
        
        # Document content
        ttk.Label(doc_frame, text="Content:").grid(row=1, column=0, sticky=(tk.W, tk.N), pady=2)
        self.doc_text = scrolledtext.ScrolledText(doc_frame, height=6, width=60, wrap=tk.WORD)
        self.doc_text.grid(row=1, column=1, columnspan=2, sticky=(tk.W, tk.E), pady=2, padx=5)
        
        # Configure RTL support for Arabic text
        self.doc_text.config(font=("Arial Unicode MS", 11))
        self.doc_text.tag_configure("rtl", justify=tk.RIGHT)
        self.doc_text.tag_configure("ltr", justify=tk.LEFT)
        
        # Default Alif content with proper RTL formatting
        sample_content = """// نموذج كود ألف
تغيير النص = "مرحبا بالعالم"
دالة اطبع_رسالة() {
    اطبع(النص)
}

اطبع_رسالة()
"""
        self.doc_text.insert(tk.END, sample_content)
        # Apply RTL formatting to the entire content
        self.doc_text.tag_add("rtl", "1.0", tk.END)
        
        # Document buttons
        doc_btn_frame = ttk.Frame(doc_frame)
        doc_btn_frame.grid(row=2, column=0, columnspan=3, pady=10, sticky=tk.W)
        
        ttk.Button(doc_btn_frame, text="Open Document", command=self.open_document, width=18).pack(side=tk.LEFT, padx=5, pady=3, ipadx=5, ipady=5)
        ttk.Button(doc_btn_frame, text="Request Completion", command=self.request_completion, width=18).pack(side=tk.LEFT, padx=5, pady=3, ipadx=5, ipady=5)
        
        # Completion position
        pos_frame = ttk.Frame(doc_frame)
        pos_frame.grid(row=3, column=0, columnspan=3, pady=5, sticky=tk.W)
        
        ttk.Label(pos_frame, text="Line:").pack(side=tk.LEFT)
        self.line_var = tk.StringVar(value="0")
        ttk.Entry(pos_frame, textvariable=self.line_var, width=5).pack(side=tk.LEFT, padx=2)
        
        ttk.Label(pos_frame, text="Character:").pack(side=tk.LEFT, padx=(10, 0))
        self.char_var = tk.StringVar(value="0")
        ttk.Entry(pos_frame, textvariable=self.char_var, width=5).pack(side=tk.LEFT, padx=2)
        
        # Response section
        response_frame = ttk.LabelFrame(main_frame, text="Server Response", padding="5")
        response_frame.grid(row=4, column=0, columnspan=3, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        response_frame.columnconfigure(0, weight=1)
        response_frame.rowconfigure(0, weight=1)
        
        self.response_text = scrolledtext.ScrolledText(response_frame, height=10, wrap=tk.WORD)
        self.response_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure RTL support for Arabic completion results
        self.response_text.config(font=("Arial Unicode MS", 10))
        self.response_text.tag_configure("rtl", justify=tk.RIGHT)
        self.response_text.tag_configure("ltr", justify=tk.LEFT)  # For English debug messages
        
        # Configure grid weights for expansion
        main_frame.rowconfigure(3, weight=1)
        main_frame.rowconfigure(4, weight=1)
        
    def browse_server(self):
        filename = filedialog.askopenfilename(
            title="Select LSP Server Executable",
            filetypes=[("Executable files", "*.exe"), ("All files", "*.*")]
        )
        if filename:
            self.server_path_var.set(filename)
    
    def start_server(self):
        server_path = self.server_path_var.get()
        if not server_path or not os.path.exists(server_path):
            messagebox.showerror("Error", "Please select a valid server executable")
            return
        
        self.client = LSPClient(server_path)
        if self.client.start_server():
            self.status_var.set("Server started")
            self.start_btn.config(state="disabled")
            self.stop_btn.config(state="normal")
            self.init_btn.config(state="normal")
            self.log_response("Server started successfully")
        else:
            messagebox.showerror("Error", "Failed to start server")
            self.status_var.set("Failed to start server")
    
    def stop_server(self):
        if self.client:
            self.client.stop_server()
            self.client = None
        
        self.status_var.set("Server stopped")
        self.start_btn.config(state="normal")
        self.stop_btn.config(state="disabled")
        self.init_btn.config(state="disabled")
        self.log_response("Server stopped")
    
    def initialize_server(self):
        if not self.client:
            messagebox.showerror("Error", "Server not started")
            return
        
        if self.client.initialize():
            self.status_var.set("Server initialized")
            self.log_response("Server initialized successfully")
        else:
            messagebox.showerror("Error", "Failed to initialize server")
            self.log_response("Failed to initialize server")
    
    def open_document(self):
        if not self.client:
            messagebox.showerror("Error", "Server not started")
            return
        
        uri = self.uri_var.get()
        content = self.doc_text.get(1.0, tk.END)
        
        self.client.open_document(uri, content)
        self.current_document_uri = uri
        self.log_response(f"Document opened: {uri}")
    
    def request_completion(self):
        if not self.client:
            messagebox.showerror("Error", "Server not started")
            return
        
        if not self.current_document_uri:
            messagebox.showerror("Error", "No document opened")
            return
        
        try:
            line = int(self.line_var.get())
            char = int(self.char_var.get())
        except ValueError:
            messagebox.showerror("Error", "Invalid line or character position")
            return
        
        self.log_response(f"Requesting completion at line {line}, character {char}")
        response = self.client.request_completion(self.current_document_uri, line, char)
        
        if response:
            self.log_response("✅ Completion response received!")
            
            if "result" in response and "items" in response["result"]:
                items = response["result"]["items"]
                self.log_response(f"Found {len(items)} completion items:")
                
                # Show first 10 items nicely formatted
                for i, item in enumerate(items[:10]):
                    label = item.get("label", "")
                    kind = item.get("kind", 0)
                    detail = item.get("detail", "")
                    self.log_response(f"  {i+1}. {label} ({detail})")
                
                if len(items) > 10:
                    self.log_response(f"  ... and {len(items) - 10} more items")
                    
                # Show full JSON in collapsible way
                self.log_response("\nFull JSON response:")
                self.log_response(json.dumps(response, indent=2, ensure_ascii=False))
            else:
                self.log_response("Response format:")
                self.log_response(json.dumps(response, indent=2, ensure_ascii=False))
                
            # Success message
            self.log_response("\n✅ Completion working perfectly! Try different positions for varied results.")
        else:
            self.log_response("❌ No completion response received")
            # Check if server is still running
            if self.client.process and self.client.process.poll() is not None:
                self.log_response(f"⚠️  Server process ended with code: {self.client.process.returncode}")
                self.status_var.set("Server crashed - restart needed")
                self.start_btn.config(state="normal")
                self.stop_btn.config(state="disabled")
                self.init_btn.config(state="disabled")
    
    def log_response(self, message):
        # Insert at end
        start_pos = self.response_text.index(tk.END + "-1c")
        self.response_text.insert(tk.END, f"{message}\n")
        end_pos = self.response_text.index(tk.END + "-1c")
        
        # Apply appropriate text direction based on content
        if self._contains_arabic(message):
            self.response_text.tag_add("rtl", start_pos, end_pos)
        else:
            self.response_text.tag_add("ltr", start_pos, end_pos)
            
        self.response_text.see(tk.END)
    
    def _contains_arabic(self, text):
        """Check if text contains Arabic characters"""
        arabic_range = range(0x0600, 0x06FF + 1)  # Arabic Unicode block
        return any(ord(char) in arabic_range for char in text)
    
    def on_closing(self):
        if self.client:
            self.client.stop_server()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = LSPTestUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()


if __name__ == "__main__":
    main()

"""
LSP Client for testing the Alif Language Server
"""
import json
import subprocess
import threading
import time
from typing import Optional, Dict, Any
import queue


class LSPClient:
    def __init__(self, server_path: str):
        self.server_path = server_path
        self.process: Optional[subprocess.Popen] = None
        self.request_id = 1
        self.response_queue = queue.Queue()
        self.is_running = False
        self.debug = False
        
    def start_server(self) -> bool:
        """Start the LSP server process"""
        try:
            self.process = subprocess.Popen(
                [self.server_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=False,  # Use binary mode to match server
                bufsize=0
            )
            self.is_running = True
            
            # Start thread to read responses
            self.response_thread = threading.Thread(target=self._read_responses, daemon=True)
            self.response_thread.start()
            
            # Start thread to read stderr
            self.stderr_thread = threading.Thread(target=self._read_stderr, daemon=True)
            self.stderr_thread.start()
            
            return True
        except Exception as e:
            print(f"Failed to start server: {e}")
            return False
    
    def stop_server(self):
        """Stop the LSP server process"""
        self.is_running = False
        if self.process:
            self.process.terminate()
            self.process.wait()
    
    def _read_responses(self):
        """Read responses from the server in a separate thread"""
        while self.is_running and self.process:
            try:
                # Read Content-Length header
                line = self.process.stdout.readline()
                if not line:
                    break
                
                line = line.decode('utf-8')
                if self.debug:
                    print(f"DEBUG: Received header: {line.strip()}")
                    
                if line.startswith("Content-Length:"):
                    length = int(line.split(":")[1].strip())
                    if self.debug:
                        print(f"DEBUG: Content length: {length}")
                    
                    # Read empty line
                    empty_line = self.process.stdout.readline()
                    if self.debug:
                        print(f"DEBUG: Empty line: {repr(empty_line)}")
                    
                    # Read the JSON content
                    content = self.process.stdout.read(length)
                    if content:
                        try:
                            content_str = content.decode('utf-8')
                            if self.debug:
                                print(f"DEBUG: Received JSON: {content_str}")
                            response = json.loads(content_str)
                            self.response_queue.put(response)
                        except json.JSONDecodeError as e:
                            print(f"Failed to parse JSON response: {e}")
                            print(f"Content: {content}")
                            
            except Exception as e:
                if self.is_running:
                    print(f"Error reading response: {e}")
                break
    
    def _read_stderr(self):
        """Read stderr from the server in a separate thread"""
        while self.is_running and self.process:
            try:
                line = self.process.stderr.readline()
                if not line:
                    break
                line = line.decode('utf-8', errors='ignore').strip()
                if line and self.debug:
                    print(f"STDERR: {line}")
            except Exception as e:
                if self.is_running:
                    print(f"Error reading stderr: {e}")
                break
    
    def send_request(self, method: str, params: Any = None) -> int:
        """Send a request to the LSP server"""
        if not self.process or not self.is_running:
            return -1
            
        request = {
            "jsonrpc": "2.0",
            "id": self.request_id,
            "method": method
        }
        
        if params is not None:
            request["params"] = params
            
        try:
            content = json.dumps(request, ensure_ascii=False)
            message = f"Content-Length: {len(content.encode('utf-8'))}\r\n\r\n{content}"
            
            if self.debug:
                print(f"DEBUG: Sending request: {message}")
            
            self.process.stdin.write(message.encode('utf-8'))
            self.process.stdin.flush()
            
            current_id = self.request_id
            self.request_id += 1
            return current_id
            
        except Exception as e:
            print(f"Failed to send request: {e}")
            return -1
    
    def send_notification(self, method: str, params: Any = None):
        """Send a notification to the LSP server (no response expected)"""
        if not self.process or not self.is_running:
            return
            
        notification = {
            "jsonrpc": "2.0",
            "method": method
        }
        
        if params is not None:
            notification["params"] = params
            
        try:
            content = json.dumps(notification, ensure_ascii=False)
            message = f"Content-Length: {len(content.encode('utf-8'))}\r\n\r\n{content}"
            
            if self.debug:
                print(f"DEBUG: Sending notification: {message}")
            
            self.process.stdin.write(message.encode('utf-8'))
            self.process.stdin.flush()
            
        except Exception as e:
            print(f"Failed to send notification: {e}")
    
    def get_response(self, timeout: float = 5.0) -> Optional[Dict[str, Any]]:
        """Get a response from the server"""
        try:
            return self.response_queue.get(timeout=timeout)
        except queue.Empty:
            return None
    
    def initialize(self, root_uri: str = "file:///") -> bool:
        """Initialize the LSP server"""
        params = {
            "processId": None,
            "rootUri": root_uri,
            "capabilities": {
                "textDocument": {
                    "completion": {
                        "completionItem": {
                            "snippetSupport": False
                        }
                    }
                }
            }
        }
        
        request_id = self.send_request("initialize", params)
        if request_id == -1:
            return False
            
        response = self.get_response()
        if response and "result" in response:
            # Send initialized notification
            self.send_notification("initialized", {})
            return True
            
        return False
    
    def open_document(self, uri: str, text: str, language_id: str = "alif"):
        """Open a document in the server"""
        params = {
            "textDocument": {
                "uri": uri,
                "languageId": language_id,
                "version": 1,
                "text": text
            }
        }
        self.send_notification("textDocument/didOpen", params)
    
    def request_completion(self, uri: str, line: int, character: int) -> Optional[Dict[str, Any]]:
        """Request completion at a specific position"""
        params = {
            "textDocument": {"uri": uri},
            "position": {"line": line, "character": character}
        }
        
        request_id = self.send_request("textDocument/completion", params)
        if request_id == -1:
            return None
            
        return self.get_response()

import os

class FlashStorage:
    def __init__(self):
        """Initialize flash storage manager"""
        # Ensure base directories exist
        self._ensure_dir('/data')
        
    def _ensure_dir(self, path):
        """Create directory if it doesn't exist"""
        try:
            os.mkdir(path)
        except OSError:
            # Directory already exists
            pass
            
    def list_files(self, directory='/data'):
        """List all files in directory"""
        try:
            return os.listdir(directory)
        except OSError as e:
            print(f"Error listing directory {directory}: {e}")
            return []
        
    def read_file(self, filename):
        """Read contents of a file"""
        try:
            with open(filename, 'r') as f:
                return f.read()
        except OSError as e:
            print(f"Error reading file {filename}: {e}")
            return None
            
    def write_file(self, filename, data):
        """Write data to a file"""
        try:
            with open(filename, 'w') as f:
                f.write(data)
            return True
        except OSError as e:
            print(f"Error writing to file {filename}: {e}")
            return False
            
    def append_file(self, filename, data):
        """Append data to a file"""
        try:
            with open(filename, 'a') as f:
                f.write(data)
            return True
        except OSError as e:
            print(f"Error appending to file {filename}: {e}")
            return False
            
    def delete_file(self, filename):
        """Delete a file"""
        try:
            os.remove(filename)
            return True
        except OSError as e:
            print(f"Error deleting file {filename}: {e}")
            return False
            
    def get_storage_info(self):
        """Get storage usage information"""
        try:
            info = os.statvfs('/')
            block_size = info[0]
            total_blocks = info[2]
            free_blocks = info[3]
            
            total_space = block_size * total_blocks
            free_space = block_size * free_blocks
            used_space = total_space - free_space
            
            return {
                'total': total_space,
                'used': used_space,
                'free': free_space,
                'percent_used': (used_space / total_space) * 100
            }
        except OSError as e:
            print(f"Error getting storage info: {e}")
            return None
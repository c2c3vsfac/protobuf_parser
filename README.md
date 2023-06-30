# protobuf_parser

需搭配proto2json使用，在不使用protoc编译中间文件的情况下反序列化字节序列。

# Using

使用时，项目结构如下所示
```
- directory
  - jsoncpp.dll
  - Parser.dll
  - packet_serialization.json
  - ucn_serialization.json
```

# python调用示例

```python
from ctypes import cdll, c_int, c_char_p
import json

dll = cdll.LoadLibrary(".\\Parser.dll")

dll.parser.restype = c_char_p

byte_str = b'J\x05H\xca\xe6\xb1\x03'
result:bytes = dll.parser(c_char_p(byte_str), c_int(len(byte_str)), c_char_p("117".encode("ascii")))
index = result.rfind(b"}")

str_result = result[:index+1].decode()
dict_result = json.loads(str_result)
```

# 其他

parser函数是没有内存泄漏吗，还是说下面这个释放内存的代码写的不对
```c++
void free_memory(const char* memory) {
    delete[] memory;
}
```
```python
dll.free_memory(c_char_p(result))
```

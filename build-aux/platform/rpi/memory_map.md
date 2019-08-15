# Virtual memory map

## 32 bit ( v6 / v7 )

```text
user area:
  0x00000000 - 0x7FFFFFFF => user process space

kernel area:
  0x80000000 - 0xBFFFFFFF => unused area
  0xC0000000 - 0xCFFFFFFF => kernel space
  0xD0000000 - 0xDFFFFFFF => kernel heap
  0xE0000000 - 0xF0FFFFFF => unused area
  0xF1000000 - 0xF1FFFFFF => temporary area
  0xF2000000 - 0xF2FFFFFF => gpio peripheral
  0xF3000000 - 0xF303FFFF => local peripheral ( rpi 2 / 3 only )
```

## 64 bit ( v8 )

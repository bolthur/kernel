# Virtual memory map

## 32 bit ( v6 / v7 )

```text
user area:
  0x00000000 - 0x7FFFFFFF => user process space
  xxxxxxxxxx - 0x7FFFF000 => downwards growing thread stacks
kernel area:
  0x80000000 - 0x9FFFFFFF => unused area
  0xA0000000 - 0xAFFFFFFF => rpc pool area
  0xB0000000 - 0xBFFFFFFF => cpu pool area
  0xC0000000 - 0xCFFFFFFF => kernel space
  0xD0000000 - 0xDFFFFFFF => kernel heap
  0xE0000000 - 0xEFFFFFFF => kasan space if enabled
  0xF0000000 - 0xF0FFFFFF => unused area
  0xF1000000 - 0xF1FFFFFF => temporary area
  0xF2000000 - 0xF2FFFFFF => gpio peripheral
  0xF3000000 - 0xF303FFFF => local peripheral ( raspi 2 / 3 only )
  0xF3040000 - 0xF3040FFF => mailbox area
  00xF341000 - 0xFFFFFFFF => process replace area
```

## 64 bit ( v8 )

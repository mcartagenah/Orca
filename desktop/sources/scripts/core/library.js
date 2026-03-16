'use strict'

/* global Operator */
/* global client */

const library = {}

library.a = function OperatorA (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'a', passive)

  this.name = 'add'
  this.info = 'Outputs sum of inputs'

  this.ports.a = { x: -1, y: 0 }
  this.ports.b = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  this.operation = function (force = false) {
    const a = this.listen(this.ports.a, true)
    const b = this.listen(this.ports.b, true)
    return orca.keyOf(a + b)
  }
}

library.b = function OperatorB (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'b', passive)

  this.name = 'subtract'
  this.info = 'Outputs difference of inputs'

  this.ports.a = { x: -1, y: 0 }
  this.ports.b = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  this.operation = function (force = false) {
    const a = this.listen(this.ports.a, true)
    const b = this.listen(this.ports.b, true)
    return orca.keyOf(Math.abs(b - a))
  }
}

library.c = function OperatorC (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'c', passive)

  this.name = 'clock'
  this.info = 'Outputs modulo of frame'

  this.ports.rate = { x: -1, y: 0, clamp: { min: 1 } }
  this.ports.mod = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  this.operation = function (force = false) {
    const rate = this.listen(this.ports.rate, true)
    const mod = this.listen(this.ports.mod, true)
    const val = Math.floor(orca.f / rate) % mod
    return orca.keyOf(val)
  }
}

library.d = function OperatorD (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'd', passive)

  this.name = 'delay'
  this.info = 'Bangs on modulo of frame'

  this.ports.rate = { x: -1, y: 0, clamp: { min: 1 } }
  this.ports.mod = { x: 1, y: 0, clamp: { min: 1 } }
  this.ports.output = { x: 0, y: 1, bang: true, output: true }

  this.operation = function (force = false) {
    const rate = this.listen(this.ports.rate, true)
    const mod = this.listen(this.ports.mod, true)
    const res = orca.f % (mod * rate)
    return res === 0 || mod === 1
  }
}

library.e = function OperatorE (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'e', passive)

  this.name = 'east'
  this.info = 'Moves eastward, or bangs'
  this.draw = false

  this.operation = function () {
    this.move(1, 0)
    this.passive = false
  }
}

library.f = function OperatorF (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'f', passive)

  this.name = 'if'
  this.info = 'Bangs if inputs are equal'

  this.ports.a = { x: -1, y: 0 }
  this.ports.b = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, bang: true, output: true }

  this.operation = function (force = false) {
    const a = this.listen(this.ports.a)
    const b = this.listen(this.ports.b)
    return a === b
  }
}

library.g = function OperatorG (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'g', passive)

  this.name = 'generator'
  this.info = 'Writes operands with offset'

  this.ports.x = { x: -3, y: 0 }
  this.ports.y = { x: -2, y: 0 }
  this.ports.len = { x: -1, y: 0, clamp: { min: 1 } }

  this.operation = function (force = false) {
    const len = this.listen(this.ports.len, true)
    const x = this.listen(this.ports.x, true)
    const y = this.listen(this.ports.y, true) + 1
    for (let offset = 0; offset < len; offset++) {
      const inPort = { x: offset + 1, y: 0 }
      const outPort = { x: x + offset, y: y, output: true }
      this.addPort(`in${offset}`, inPort)
      this.addPort(`out${offset}`, outPort)
      const res = this.listen(inPort)
      this.output(`${res}`, outPort)
    }
  }
}

library.h = function OperatorH (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'h', passive)

  this.name = 'halt'
  this.info = 'Halts southward operand'

  this.ports.output = { x: 0, y: 1, reader: true, output: true }

  this.operation = function (force = false) {
    orca.lock(this.x + this.ports.output.x, this.y + this.ports.output.y)
    return this.listen(this.ports.output, true)
  }
}

library.i = function OperatorI (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'i', passive)

  this.name = 'increment'
  this.info = 'Increments southward operand'

  this.ports.step = { x: -1, y: 0 }
  this.ports.mod = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, reader: true, output: true }

  this.operation = function (force = false) {
    const step = this.listen(this.ports.step, true)
    const mod = this.listen(this.ports.mod, true)
    const val = this.listen(this.ports.output, true)
    return mod ? orca.keyOf((val + step) % mod) : '0'
  }
}

library.j = function OperatorJ (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'j', passive)

  this.name = 'jumper'
  this.info = 'Outputs northward operand'

  this.operation = function (force = false) {
    const val = this.listen({ x: 0, y: -1 })
    if (val != "J") {
      let i = 0
      while (orca.inBounds(this.x, this.y + i)) {
        if (this.listen({ x: 0, y: ++i }) != this.glyph) { break }
      }
      this.addPort('input', { x: 0, y: -1 })
      this.addPort('output', { x: 0, y: i, output: true })
      return val
    }
  }
}

library.k = function OperatorK (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'k', passive)

  this.name = 'konkat'
  this.info = 'Reads multiple variables'

  this.ports.len = { x: -1, y: 0, clamp: { min: 1 } }

  this.operation = function (force = false) {
    this.len = this.listen(this.ports.len, true)
    for (let offset = 0; offset < this.len; offset++) {
      const key = orca.glyphAt(this.x + offset + 1, this.y)
      orca.lock(this.x + offset + 1, this.y)
      if (key === '.') { continue }
      const inPort = { x: offset + 1, y: 0 }
      const outPort = { x: offset + 1, y: 1, output: true }
      this.addPort(`in${offset}`, inPort)
      this.addPort(`out${offset}`, outPort)
      const res = orca.valueIn(key)
      this.output(`${res}`, outPort)
    }
  }
}

library.l = function OperatorL (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'l', passive)

  this.name = 'lesser'
  this.info = 'Outputs smallest input'

  this.ports.a = { x: -1, y: 0 }
  this.ports.b = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  this.operation = function (force = false) {
    const a = this.listen(this.ports.a, true)
    const b = this.listen(this.ports.b, true)
    return orca.keyOf(a > b ? b : a)
  }
}

library.m = function OperatorM (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'm', passive)

  this.name = 'multiply'
  this.info = 'Outputs product of inputs'

  this.ports.a = { x: -1, y: 0 }
  this.ports.b = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  this.operation = function (force = false) {
    const a = this.listen(this.ports.a, true)
    const b = this.listen(this.ports.b, true)
    return orca.keyOf(a * b)
  }
}

library.n = function OperatorN (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'n', passive)

  this.name = 'north'
  this.info = 'Moves Northward, or bangs'
  this.draw = false

  this.operation = function () {
    this.move(0, -1)
    this.passive = false
  }
}

library.o = function OperatorO (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'o', passive)

  this.name = 'read'
  this.info = 'Reads operand with offset'

  this.ports.x = { x: -2, y: 0 }
  this.ports.y = { x: -1, y: 0 }
  this.ports.output = { x: 0, y: 1, output: true }

  this.operation = function (force = false) {
    const x = this.listen(this.ports.x, true)
    const y = this.listen(this.ports.y, true)
    this.addPort('read', { x: x + 1, y: y })
    return this.listen(this.ports.read)
  }
}

library.p = function OperatorP (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'p', passive)

  this.name = 'push'
  this.info = 'Writes eastward operand'

  this.ports.key = { x: -2, y: 0 }
  this.ports.len = { x: -1, y: 0, clamp: { min: 1 } }
  this.ports.val = { x: 1, y: 0 }

  this.operation = function (force = false) {
    const len = this.listen(this.ports.len, true)
    const key = this.listen(this.ports.key, true)
    for (let offset = 0; offset < len; offset++) {
      orca.lock(this.x + offset, this.y + 1)
    }
    this.ports.output = { x: (key % len), y: 1, output: true }
    return this.listen(this.ports.val)
  }
}

library.q = function OperatorQ (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'q', passive)

  this.name = 'query'
  this.info = 'Reads operands with offset'

  this.ports.x = { x: -3, y: 0 }
  this.ports.y = { x: -2, y: 0 }
  this.ports.len = { x: -1, y: 0, clamp: { min: 1 } }

  this.operation = function (force = false) {
    const len = this.listen(this.ports.len, true)
    const x = this.listen(this.ports.x, true)
    const y = this.listen(this.ports.y, true)
    for (let offset = 0; offset < len; offset++) {
      const inPort = { x: x + offset + 1, y: y }
      const outPort = { x: offset - len + 1, y: 1, output: true }
      this.addPort(`in${offset}`, inPort)
      this.addPort(`out${offset}`, outPort)
      const res = this.listen(inPort)
      this.output(`${res}`, outPort)
    }
  }
}

library.r = function OperatorR (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'r', passive)

  this.name = 'random'
  this.info = 'Outputs random value'

  this.ports.a = { x: -1, y: 0 }
  this.ports.b = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  this.operation = function (force = false) {
    const a = this.listen(this.ports.a, true)
    const b = this.listen(this.ports.b, true)
    if(a == b)
        return orca.keyOf(a)
    if(a > b)
        return orca.keyOf(parseInt(Math.random() * (a - b + 1) + b))
    return orca.keyOf(parseInt(Math.random() * (b - a + 1) + a))
  }
}

library.s = function OperatorS (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 's', passive)

  this.name = 'south'
  this.info = 'Moves southward, or bangs'
  this.draw = false

  this.operation = function () {
    this.move(0, 1)
    this.passive = false
  }
}

library.t = function OperatorT (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 't', passive)

  this.name = 'track'
  this.info = 'Reads eastward operand'

  this.ports.key = { x: -2, y: 0 }
  this.ports.len = { x: -1, y: 0, clamp: { min: 1 } }
  this.ports.output = { x: 0, y: 1, output: true }

  this.operation = function (force = false) {
    const len = this.listen(this.ports.len, true)
    const key = this.listen(this.ports.key, true)
    for (let offset = 0; offset < len; offset++) {
      orca.lock(this.x + offset + 1, this.y)
    }
    this.ports.val = { x: (key % len) + 1, y: 0 }
    return this.listen(this.ports.val)
  }
}

library.u = function OperatorU (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'u', passive)

  this.name = 'uclid'
  this.info = 'Bangs on Euclidean rhythm'

  this.ports.step = { x: -1, y: 0, clamp: { min: 0 } }
  this.ports.max = { x: 1, y: 0, clamp: { min: 1 } }
  this.ports.output = { x: 0, y: 1, bang: true, output: true }

  this.operation = function (force = false) {
    const step = this.listen(this.ports.step, true)
    const max = this.listen(this.ports.max, true)
    const bucket = (step * (orca.f + max - 1)) % max + step
    return bucket >= max
  }
}

library.v = function OperatorV (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'v', passive)

  this.name = 'variable'
  this.info = 'Reads and writes variable'

  this.ports.write = { x: -1, y: 0 }
  this.ports.read = { x: 1, y: 0 }

  this.operation = function (force = false) {
    const write = this.listen(this.ports.write)
    const read = this.listen(this.ports.read)
    if (write === '.' && read !== '.') {
      this.addPort('output', { x: 0, y: 1, output: true })
    }
    if (write !== '.') {
      orca.variables[write] = read
      return
    }
    return orca.valueIn(read)
  }
}

library.w = function OperatorW (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'w', passive)

  this.name = 'west'
  this.info = 'Moves westward, or bangs'
  this.draw = false

  this.operation = function () {
    this.move(-1, 0)
    this.passive = false
  }
}

library.x = function OperatorX (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'x', passive)

  this.name = 'write'
  this.info = 'Writes operand with offset'

  this.ports.x = { x: -2, y: 0 }
  this.ports.y = { x: -1, y: 0 }
  this.ports.val = { x: 1, y: 0 }

  this.operation = function (force = false) {
    const x = this.listen(this.ports.x, true)
    const y = this.listen(this.ports.y, true) + 1
    this.addPort('output', { x: x, y: y, output: true })
    return this.listen(this.ports.val)
  }
}

library.y = function OperatorY (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'y', passive)

  this.name = 'jymper'
  this.info = 'Outputs westward operand'

  this.operation = function (force = false) {
    const val = this.listen({ x: -1, y: 0, output: true })
    if (val != "Y") {
      let i = 0
      while (orca.inBounds(this.x + i, this.y)) {
        if (this.listen({ x: ++i, y: 0 }) != this.glyph) { break }
      }
      this.addPort('input', { x: -1, y: 0 })
      this.addPort('output', { x: i, y: 0, output: true })
      return val
    }
  }
}

library.z = function OperatorZ (orca, x, y, passive) {
  Operator.call(this, orca, x, y, 'z', passive)

  this.name = 'lerp'
  this.info = 'Transitions operand to target'

  this.ports.rate = { x: -1, y: 0 }
  this.ports.target = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, reader: true, output: true }

  this.operation = function (force = false) {
    const rate = this.listen(this.ports.rate, true)
    const target = this.listen(this.ports.target, true)
    const val = this.listen(this.ports.output, true)
    const mod = val <= target - rate ? rate : val >= target + rate ? -rate : target - val
    return orca.keyOf(val + mod)
  }
}

// Specials

library['*'] = function OperatorBang (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '*', true)

  this.name = 'bang'
  this.info = 'Bangs neighboring operands'
  this.draw = false

  this.run = function (force = false) {
    this.draw = false
    this.erase()
  }
}

library['#'] = function OperatorComment (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '#', true)

  this.name = 'comment'
  this.info = 'Halts line'
  this.draw = false

  this.operation = function () {
    for (let x = this.x + 1; x <= orca.w; x++) {
      orca.lock(x, this.y)
      if (orca.glyphAt(x, this.y) === this.glyph) { break }
    }
    orca.lock(this.x, this.y)
  }
}

// IO

library.$ = function OperatorSelf (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '*', true)

  this.name = 'self'
  this.info = 'Sends ORCA command'

  this.run = function (force = false) {
    let msg = ''
    for (let x = 1; x <= 36; x++) {
      const g = orca.glyphAt(this.x + x, this.y)
      orca.lock(this.x + x, this.y)
      if (g === '.') { break }
      msg += g
    }

    if (!this.hasNeighbor('*') && force === false) { return }
    if (msg === '') { return }

    this.draw = false
    client.commander.trigger(`${msg}`, { x, y: y + 1 }, false)
  }
}

library[':'] = function OperatorMidi (orca, x, y, passive) {
  Operator.call(this, orca, x, y, ':', true)

  this.name = 'midi'
  this.info = 'Sends MIDI note'
  this.ports.channel = { x: 1, y: 0 }
  this.ports.octave = { x: 2, y: 0, clamp: { min: 0, max: 8 } }
  this.ports.note = { x: 3, y: 0 }
  this.ports.velocity = { x: 4, y: 0, default: 'f', clamp: { min: 0, max: 16 } }
  this.ports.length = { x: 5, y: 0, default: '1', clamp: { min: 0, max: 32 } }

  this.operation = function (force = false) {
    if (!this.hasNeighbor('*') && force === false) { return }
    if (this.listen(this.ports.channel) === '.') { return }
    if (this.listen(this.ports.octave) === '.') { return }
    if (this.listen(this.ports.note) === '.') { return }
    if (!isNaN(this.listen(this.ports.note))) { return }

    const channel = this.listen(this.ports.channel, true)
    if (channel > 15) { return }
    const octave = this.listen(this.ports.octave, true)
    const note = this.listen(this.ports.note)
    const velocity = this.listen(this.ports.velocity, true)
    const length = this.listen(this.ports.length, true)

    client.io.midi.push(channel, octave, note, velocity, length)

    if (force === true) {
      client.io.midi.run()
    }

    this.draw = false
  }
}

library['!'] = function OperatorCC (orca, x, y) {
  Operator.call(this, orca, x, y, '!', true)

  this.name = 'cc'
  this.info = 'Sends MIDI control change'
  this.ports.channel = { x: 1, y: 0 }
  this.ports.knob = { x: 2, y: 0, clamp: { min: 0 } }
  this.ports.value = { x: 3, y: 0, clamp: { min: 0 } }

  this.operation = function (force = false) {
    if (!this.hasNeighbor('*') && force === false) { return }
    if (this.listen(this.ports.channel) === '.') { return }
    if (this.listen(this.ports.knob) === '.') { return }

    const channel = this.listen(this.ports.channel, true)
    if (channel > 15) { return }
    const knob = this.listen(this.ports.knob, true)
    const rawValue = this.listen(this.ports.value, true)
    const value = Math.ceil((127 * rawValue) / 35)

    client.io.cc.stack.push({ channel, knob, value, type: 'cc' })

    this.draw = false

    if (force === true) {
      client.io.cc.run()
    }
  }
}

library['?'] = function OperatorPB (orca, x, y) {
  Operator.call(this, orca, x, y, '?', true)

  this.name = 'pb'
  this.info = 'Sends MIDI pitch bend'
  this.ports.channel = { x: 1, y: 0, clamp: { min: 0, max: 15 } }
  this.ports.lsb = { x: 2, y: 0, clamp: { min: 0 } }
  this.ports.msb = { x: 3, y: 0, clamp: { min: 0 } }

  this.operation = function (force = false) {
    if (!this.hasNeighbor('*') && force === false) { return }
    if (this.listen(this.ports.channel) === '.') { return }
    if (this.listen(this.ports.lsb) === '.') { return }

    const channel = this.listen(this.ports.channel, true)
    const rawlsb = this.listen(this.ports.lsb, true)
    const lsb = Math.ceil((127 * rawlsb) / 35)
    const rawmsb = this.listen(this.ports.msb, true)
    const msb = Math.ceil((127 * rawmsb) / 35)

    client.io.cc.stack.push({ channel, lsb, msb, type: 'pb' })

    this.draw = false

    if (force === true) {
      client.io.cc.run()
    }
  }
}

library['%'] = function OperatorMono (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '%', true)

  this.name = 'mono'
  this.info = 'Sends MIDI monophonic note'
  this.ports.channel = { x: 1, y: 0 }
  this.ports.octave = { x: 2, y: 0, clamp: { min: 0, max: 8 } }
  this.ports.note = { x: 3, y: 0 }
  this.ports.velocity = { x: 4, y: 0, default: 'f', clamp: { min: 0, max: 16 } }
  this.ports.length = { x: 5, y: 0, default: '1', clamp: { min: 0, max: 32 } }

  this.operation = function (force = false) {
    if (!this.hasNeighbor('*') && force === false) { return }
    if (this.listen(this.ports.channel) === '.') { return }
    if (this.listen(this.ports.octave) === '.') { return }
    if (this.listen(this.ports.note) === '.') { return }
    if (!isNaN(this.listen(this.ports.note))) { return }

    const channel = this.listen(this.ports.channel, true)
    if (channel > 15) { return }
    const octave = this.listen(this.ports.octave, true)
    const note = this.listen(this.ports.note)
    const velocity = this.listen(this.ports.velocity, true)
    const length = this.listen(this.ports.length, true)

    client.io.mono.push(channel, octave, note, velocity, length)

    if (force === true) {
      client.io.mono.run()
    }

    this.draw = false
  }
}

library['='] = function OperatorOsc (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '=', true)

  this.name = 'osc'
  this.info = 'Sends OSC message'

  this.ports.path = { x: 1, y: 0 }

  this.operation = function (force = false) {
    let msg = ''
    for (let x = 2; x <= 36; x++) {
      const g = orca.glyphAt(this.x + x, this.y)
      orca.lock(this.x + x, this.y)
      if (g === '.') { break }
      msg += g
    }

    if (!this.hasNeighbor('*') && force === false) { return }

    const path = this.listen(this.ports.path)

    if (!path || path === '.') { return }

    this.draw = false
    client.io.osc.push('/' + path, msg)

    if (force === true) {
      client.io.osc.run()
    }
  }
}

library[';'] = function OperatorUdp (orca, x, y, passive) {
  Operator.call(this, orca, x, y, ';', true)

  this.name = 'udp'
  this.info = 'Sends UDP message'

  this.operation = function (force = false) {
    let msg = ''
    for (let x = 1; x <= 36; x++) {
      const g = orca.glyphAt(this.x + x, this.y)
      orca.lock(this.x + x, this.y)
      if (g === '.') { break }
      msg += g
    }

    if (!this.hasNeighbor('*') && force === false) { return }

    this.draw = false
    client.io.udp.push(msg)

    if (force === true) {
      client.io.udp.run()
    }
  }
}

// Generative

library['~'] = function OperatorProbability (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '~', true)

  this.name = 'probability'
  this.info = 'Bangs with probability'

  this.ports.chance = { x: 1, y: 0, clamp: { min: 0, max: 35 } }
  this.ports.output = { x: 0, y: 1, bang: true, output: true }

  this.operation = function (force = false) {
    // Check for bang neighbor excluding south (output port) to avoid self-triggering
    const hasBangInput = orca.glyphAt(this.x - 1, this.y) === '*' ||
                         orca.glyphAt(this.x + 1, this.y) === '*' ||
                         orca.glyphAt(this.x, this.y - 1) === '*'
    if (!hasBangInput && force === false) { return false }
    const chance = this.listen(this.ports.chance, true)
    const roll = parseInt(Math.random() * 36)
    return roll <= chance
  }
}

library['^'] = function OperatorScale (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '^', true)

  this.name = 'scale'
  this.info = 'Quantizes to musical scale'

  this.ports.note = { x: -1, y: 0 }
  this.ports.scale = { x: 1, y: 0, clamp: { min: 0, max: 7 } }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  const scales = [
    [0,1,2,3,4,5,6,7,8,9,10,11], // chromatic
    [0,2,4,5,7,9,11],             // major
    [0,2,3,5,7,8,10],             // minor
    [0,2,4,7,9],                   // pentatonic
    [0,3,5,6,7,10],               // blues
    [0,2,3,5,7,9,10],             // dorian
    [0,2,4,5,7,9,10],             // mixolydian
    [0,2,3,5,7,8,11]              // harmonic minor
  ]

  this.operation = function (force = false) {
    const noteVal = this.listen(this.ports.note, true)
    const scaleId = this.listen(this.ports.scale, true)
    const scale = scales[Math.min(scaleId, 7)]
    const octave = Math.floor(noteVal / 12)
    const semitone = noteVal % 12
    let bestDist = 99, bestNote = 0
    for (let i = 0; i < scale.length; i++) {
      const dist = Math.abs(semitone - scale[i])
      if (dist < bestDist) {
        bestDist = dist
        bestNote = scale[i]
      }
    }
    return orca.keyOf(octave * 12 + bestNote)
  }
}

library['{'] = function OperatorBuffer (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '{', true)

  this.name = 'buffer'
  this.info = 'Shift register'

  this.ports.len = { x: -1, y: 0, clamp: { min: 1 } }
  this.ports.val = { x: 1, y: 0 }

  this.operation = function (force = false) {
    const len = this.listen(this.ports.len, true)
    const newVal = this.listen(this.ports.val)
    const banged = this.hasNeighbor('*')

    // Read current south row
    const buf = []
    for (let i = 0; i < len; i++) {
      buf.push(orca.glyphAt(this.x + i, this.y + 1))
    }

    if (banged) {
      // Shift right, insert new value at [0]
      for (let i = len - 1; i > 0; i--) {
        const outPort = { x: i, y: 1, output: true }
        this.addPort(`out${i}`, outPort)
        this.output(`${buf[i - 1]}`, outPort)
      }
      const outPort = { x: 0, y: 1, output: true }
      this.addPort('out0', outPort)
      this.output(`${newVal}`, outPort)
    }

    // Lock south row
    for (let i = 0; i < len; i++) {
      orca.lock(this.x + i, this.y + 1)
    }
  }
}

library['}'] = function OperatorFreeze (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '}', true)

  this.name = 'freeze'
  this.info = 'Sample and hold'

  this.ports.val = { x: -1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, reader: true, output: true }

  this.operation = function (force = false) {
    const hasBang = (orca.glyphAt(this.x - 1, this.y) === '*') ||
                    (orca.glyphAt(this.x + 1, this.y) === '*') ||
                    (orca.glyphAt(this.x, this.y - 1) === '*')
    if (hasBang) {
      return this.listen(this.ports.val)
    }
    return this.listen(this.ports.output)
  }
}

library['|'] = function OperatorGate (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '|', true)

  this.name = 'gate'
  this.info = 'Passes value if above threshold'

  this.ports.threshold = { x: -1, y: 0 }
  this.ports.val = { x: 1, y: 0 }
  this.ports.output = { x: 0, y: 1, sensitive: true, output: true }

  this.operation = function (force = false) {
    const threshold = this.listen(this.ports.threshold, true)
    const val = this.listen(this.ports.val, true)
    const rawVal = this.listen(this.ports.val)
    return val >= threshold ? rawVal : '.'
  }
}

library['&'] = function OperatorArp (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '&', true)

  this.name = 'arp'
  this.info = 'Arpeggiator'

  this.ports.speed = { x: -2, y: 0, clamp: { min: 1 } }
  this.ports.pattern = { x: -1, y: 0, clamp: { min: 0, max: 3 } }
  this.ports.len = { x: 1, y: 0, clamp: { min: 1 } }

  this.operation = function (force = false) {
    const speed = this.listen(this.ports.speed, true)
    const pattern = this.listen(this.ports.pattern, true)
    const len = this.listen(this.ports.len, true)
    if (len <= 0) return

    // Read and lock notes eastward
    const notes = []
    for (let i = 0; i < len; i++) {
      notes.push(orca.glyphAt(this.x + 2 + i, this.y))
      orca.lock(this.x + 2 + i, this.y)
      this.addPort(`in${i}`, { x: 2 + i, y: 0 })
    }

    const step = Math.floor(orca.f / speed)
    let index = 0
    switch (pattern) {
      case 0: index = step % len; break // up
      case 1: index = (len - 1) - (step % len); break // down
      case 2: { // updown
        const cycle = len > 1 ? 2 * (len - 1) : 1
        const pos = step % cycle
        index = pos < len ? pos : cycle - pos
        break
      }
      default: index = parseInt(Math.random() * len); break // random
    }

    this.addPort('output', { x: 0, y: 1, sensitive: true, output: true })
    return notes[index]
  }
}

library['@'] = function OperatorMarkov (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '@', true)

  this.name = 'markov'
  this.info = 'State machine sequencer'

  this.ports.len = { x: 1, y: 0, clamp: { min: 1 } }
  this.ports.output = { x: 0, y: 1, sensitive: true, reader: true, output: true }

  this.operation = function (force = false) {
    const len = this.listen(this.ports.len, true)
    if (len <= 0) return

    // Lock state table eastward
    for (let i = 0; i < len; i++) {
      orca.lock(this.x + 2 + i, this.y)
      this.addPort(`in${i}`, { x: 2 + i, y: 0 })
    }

    const hasBang = (orca.glyphAt(this.x - 1, this.y) === '*') ||
                    (orca.glyphAt(this.x + 1, this.y) === '*') ||
                    (orca.glyphAt(this.x, this.y - 1) === '*')
    if (hasBang) {
      const state = this.listen(this.ports.output, true)
      const idx = state % len
      const next = orca.glyphAt(this.x + 2 + idx, this.y)
      return next === '.' ? orca.keyOf(0) : next
    }
    return this.listen(this.ports.output)
  }
}

library['['] = function OperatorStrum (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '[', true)

  this.name = 'strum'
  this.info = 'Outputs sequential bangs southward'

  this.ports.len = { x: -1, y: 0, clamp: { min: 1, max: 35 } }
  this.ports.rate = { x: 1, y: 0, default: '1', clamp: { min: 1, max: 35 } }

  this.operation = function (force = false) {
    const len = this.listen(this.ports.len, true)
    let rate = this.listen(this.ports.rate, true)
    if (len <= 0) return
    if (rate <= 0) rate = 1

    // Lock state cells (position counter + rate countdown)
    orca.lock(this.x, this.y + 1)
    orca.lock(this.x, this.y + 2)
    // Lock output cells
    for (let i = 0; i < len; i++) orca.lock(this.x, this.y + 3 + i)

    let position = orca.valueOf(orca.glyphAt(this.x, this.y + 1))
    let rateCount = orca.valueOf(orca.glyphAt(this.x, this.y + 2))

    const hasBang = (orca.glyphAt(this.x - 1, this.y) === '*') ||
                    (orca.glyphAt(this.x + 1, this.y) === '*') ||
                    (orca.glyphAt(this.x, this.y - 1) === '*')

    if (hasBang && position === 0) {
      position = len
      rateCount = 1 // bang immediately on first frame
    }

    if (position > 0) {
      rateCount--
      if (rateCount <= 0) {
        // Bang at current position
        const activePos = len - position
        for (let i = 0; i < len; i++) {
          orca.write(this.x, this.y + 3 + i, i === activePos ? '*' : '.')
        }
        position--
        rateCount = rate
      } else {
        // Between bangs — all dots
        for (let i = 0; i < len; i++) {
          orca.write(this.x, this.y + 3 + i, '.')
        }
      }
      orca.write(this.x, this.y + 1, position > 0 ? orca.keyOf(position) : orca.keyOf(0))
      orca.write(this.x, this.y + 2, rateCount > 0 ? orca.keyOf(rateCount) : orca.keyOf(0))
    } else {
      // Idle
      orca.write(this.x, this.y + 1, '.')
      orca.write(this.x, this.y + 2, '.')
      for (let i = 0; i < len; i++) {
        orca.write(this.x, this.y + 3 + i, '.')
      }
    }
  }
}

library[']'] = function OperatorChord (orca, x, y, passive) {
  Operator.call(this, orca, x, y, ']', true)

  this.name = 'chord'
  this.info = 'Outputs chord notes southward'

  this.ports.root = { x: -1, y: 0 }
  this.ports.type = { x: -2, y: 0, clamp: { min: 0, max: 8 } }

  this.operation = function (force = false) {
    const chordSizes = [3, 3, 3, 3, 3, 3, 4, 4, 4]
    const chords = [
      [0, 4, 7, 0],   // 0 = major
      [0, 3, 7, 0],   // 1 = minor
      [0, 3, 6, 0],   // 2 = dim
      [0, 4, 8, 0],   // 3 = aug
      [0, 2, 7, 0],   // 4 = sus2
      [0, 5, 7, 0],   // 5 = sus4
      [0, 4, 7, 11],  // 6 = maj7
      [0, 3, 7, 10],  // 7 = min7
      [0, 4, 7, 10],  // 8 = dom7
    ]
    const naturalSemitones = [9, 11, 0, 2, 4, 5, 7] // A B C D E F G
    const semitoneToNote = ['C', 'c', 'D', 'd', 'E', 'F', 'f', 'G', 'g', 'A', 'a', 'B']

    // Always lock up to 4 output cells to hold last chord on invalid input
    for (let i = 0; i < 4; i++) {
      orca.lock(this.x, this.y + 1 + i)
    }

    const rootGlyph = this.listen(this.ports.root)
    if (!rootGlyph.match(/[a-zA-Z]/)) return // hold last chord (cells already locked)

    let chordType = this.listen(this.ports.type, true)
    if (chordType > 8) chordType = 8
    const size = chordSizes[chordType]

    // Convert root letter to semitone
    const baseIdx = (rootGlyph.toLowerCase().charCodeAt(0) - 97) % 7
    let rootSemitone = naturalSemitones[baseIdx]
    if (rootGlyph === rootGlyph.toLowerCase()) rootSemitone = (rootSemitone + 1) % 12 // lowercase = sharp

    for (let i = 0; i < size; i++) {
      const semitone = (rootSemitone + chords[chordType][i]) % 12
      this.addPort(`out${i}`, { x: 0, y: 1 + i, output: true })
      orca.write(this.x, this.y + 1 + i, semitoneToNote[semitone])
    }
    // Clear unused cells (e.g., switching from 7th to triad)
    for (let i = size; i < 4; i++) {
      orca.write(this.x, this.y + 1 + i, '.')
    }
  }
}

library['>'] = function OperatorHumanize (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '>', true)

  this.name = 'humanize'
  this.info = 'Delays bangs by a random number of frames'

  this.ports.max = { x: 1, y: 0, clamp: { min: 0, max: 35 } }
  this.ports.counter = { x: 0, y: 1, reader: true, output: true }

  this.operation = function (force = false) {
    orca.lock(this.x, this.y + 1)
    orca.lock(this.x, this.y + 2)

    const maxDelay = this.listen(this.ports.max, true)
    const counter = this.listen(this.ports.counter, true)

    const hasBang = (orca.glyphAt(this.x - 1, this.y) === '*') ||
                    (orca.glyphAt(this.x + 1, this.y) === '*') ||
                    (orca.glyphAt(this.x, this.y - 1) === '*')

    if (hasBang && counter === 0) {
      const delay = Math.floor(Math.random() * (maxDelay + 1))
      if (delay === 0) {
        orca.write(this.x, this.y + 1, '.')
        orca.write(this.x, this.y + 2, '*')
      } else {
        orca.write(this.x, this.y + 1, orca.keyOf(delay))
        orca.write(this.x, this.y + 2, '.')
      }
    } else if (counter > 0) {
      const next = counter - 1
      if (next === 0) {
        orca.write(this.x, this.y + 1, orca.keyOf(0))
        orca.write(this.x, this.y + 2, '*')
      } else {
        orca.write(this.x, this.y + 1, orca.keyOf(next))
        orca.write(this.x, this.y + 2, '.')
      }
    } else {
      orca.write(this.x, this.y + 1, '.')
      orca.write(this.x, this.y + 2, '.')
    }
  }
}

library['<'] = function OperatorRatchet (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '<', true)

  this.name = 'ratchet'
  this.info = 'Subdivides a bang into multiple evenly-spaced bangs'

  this.ports.subs = { x: 1, y: 0, clamp: { min: 1, max: 9 } }
  this.ports.period = { x: 2, y: 0, clamp: { min: 1, max: 36 } }
  this.ports.counter = { x: 0, y: 1, reader: true, output: true }

  this.operation = function (force = false) {
    orca.lock(this.x, this.y + 1)
    orca.lock(this.x, this.y + 2)

    const subs = this.listen(this.ports.subs, true)
    const period = this.listen(this.ports.period, true)
    let counter = this.listen(this.ports.counter, true)

    const hasBang = (orca.glyphAt(this.x - 1, this.y) === '*') ||
                    (orca.glyphAt(this.x + 1, this.y) === '*') ||
                    (orca.glyphAt(this.x, this.y - 1) === '*')

    if (hasBang && counter === 0) {
      counter = period
    }

    if (counter > 0) {
      const pos = period - counter
      let shouldBang = false
      if (pos === 0) {
        shouldBang = true
      } else if (subs > 0 && period > 0) {
        const prevSlot = Math.floor((pos - 1) * subs / period)
        const curSlot = Math.floor(pos * subs / period)
        shouldBang = (curSlot !== prevSlot)
      }
      counter--
      orca.write(this.x, this.y + 1, counter > 0 ? orca.keyOf(counter) : orca.keyOf(0))
      orca.write(this.x, this.y + 2, shouldBang ? '*' : '.')
    } else {
      orca.write(this.x, this.y + 1, '.')
      orca.write(this.x, this.y + 2, '.')
    }
  }
}

library['\\'] = function OperatorSwingGate (orca, x, y, passive) {
  Operator.call(this, orca, x, y, '\\', true)

  this.name = 'swing'
  this.info = 'Alternates between immediate and delayed bangs'

  this.ports.delay = { x: 1, y: 0, clamp: { min: 0, max: 35 } }
  this.ports.toggle = { x: 0, y: 1, reader: true, output: true }
  this.ports.countdown = { x: 0, y: 2, reader: true, output: true }

  this.operation = function (force = false) {
    orca.lock(this.x, this.y + 1)
    orca.lock(this.x, this.y + 2)
    orca.lock(this.x, this.y + 3)

    const delay = this.listen(this.ports.delay, true)
    let toggle = this.listen(this.ports.toggle, true)
    let countdown = this.listen(this.ports.countdown, true)

    const hasBang = (orca.glyphAt(this.x - 1, this.y) === '*') ||
                    (orca.glyphAt(this.x + 1, this.y) === '*') ||
                    (orca.glyphAt(this.x, this.y - 1) === '*')

    let bangOut = false

    if (hasBang && countdown === 0) {
      toggle = toggle ? 0 : 1
      if (toggle === 1) {
        bangOut = true
      } else {
        if (delay === 0) {
          bangOut = true
        } else {
          countdown = delay
        }
      }
    }

    if (countdown > 0 && !bangOut) {
      countdown--
      if (countdown === 0) {
        bangOut = true
      }
    }

    orca.write(this.x, this.y + 1, orca.keyOf(toggle))
    orca.write(this.x, this.y + 2, countdown > 0 ? orca.keyOf(countdown) : orca.keyOf(0))
    orca.write(this.x, this.y + 3, bangOut ? '*' : '.')
  }
}

// Add numbers

for (let i = 0; i <= 9; i++) {
  library[`${i}`] = function OperatorNull (orca, x, y, passive) {
    Operator.call(this, orca, x, y, '.', false)

    this.name = 'null'
    this.info = 'empty'

    // Overwrite run, to disable draw.
    this.run = function (force = false) {

    }
  }
}

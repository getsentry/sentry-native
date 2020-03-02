import subprocess
import os
import io
import json
import sys

def run(cwd, exe, args, **kwargs):
    cmd = "./{}".format(exe) if sys.platform != "win32" else "{}\\{}.exe".format(cwd, exe)
    return subprocess.run([cmd, *args], cwd=cwd, **kwargs)

def check_output(*args):
    stdout = run(*args, check=True, stdout=subprocess.PIPE).stdout
    # capturing stdout on windows actually encodes "\n" as "\r\n", which we
    # revert, because it messes with envelope decoding
    stdout = stdout.replace(b"\r\n", b"\n")
    return stdout

def cmake(cwd, targets, options=[]):
    configcmd = ["cmake"]
    for option in options:
        configcmd.extend(["-D", option])
    configcmd.append(os.getcwd())
    subprocess.run(configcmd, cwd=cwd, check=True)

    buildcmd = ["cmake", "--build", ".", "--parallel"]
    for target in targets:
        buildcmd.extend(["--target", target])
    subprocess.run(buildcmd, cwd=cwd, check=True)

# Adapted from: https://raw.githubusercontent.com/getsentry/sentry-python/276acae955ee13f7ac3a7728003626eff6d943a8/sentry_sdk/envelope.py

class Envelope(object):
    def __init__(
        self,
        headers=None,  # type: Optional[Dict[str, str]]
        items=None,  # type: Optional[List[Item]]
    ):
        # type: (...) -> None
        if headers is not None:
            headers = dict(headers)
        self.headers = headers or {}
        if items is None:
            items = []
        else:
            items = list(items)
        self.items = items

    def get_event(self):
        # type: (...) -> Optional[Event]
        for items in self.items:
            event = items.get_event()
            if event is not None:
                return event
        return None

    def __iter__(self):
        # type: (...) -> Iterator[Item]
        return iter(self.items)

    @classmethod
    def deserialize_from(
        cls, f  # type: Any
    ):
        # type: (...) -> Envelope
        headers = json.loads(f.readline())
        items = []
        while 1:
            item = Item.deserialize_from(f)
            if item is None:
                break
            items.append(item)
        return cls(headers=headers, items=items)

    @classmethod
    def deserialize(
        cls, bytes  # type: bytes
    ):
        # type: (...) -> Envelope
        return cls.deserialize_from(io.BytesIO(bytes))

    def __repr__(self):
        # type: (...) -> str
        return "<Envelope headers=%r items=%r>" % (self.headers, self.items)

class PayloadRef(object):
    def __init__(
        self,
        bytes=None,  # type: Optional[bytes]
        json=None,  # type: Optional[Any]
    ):
        # type: (...) -> None
        self.json = json
        self.bytes = bytes

    def __repr__(self):
        # type: (...) -> str
        return "<Payload bytes=%r json=%r>" % (self.bytes, self.json)

class Item(object):
    def __init__(
        self,
        payload,  # type: Union[bytes, text_type, PayloadRef]
        headers=None,  # type: Optional[Dict[str, str]]
    ):
        if headers is not None:
            headers = dict(headers)
        elif headers is None:
            headers = {}
        self.headers = headers
        if isinstance(payload, bytes):
            payload = PayloadRef(bytes=payload)
        else:
            payload = payload

        self.payload = payload

    def get_event(self):
        # type: (...) -> Optional[Event]
        if self.headers.get("type") == "event" and self.payload.json is not None:
            return self.payload.json
        return None

    @classmethod
    def deserialize_from(
        cls, f  # type: Any
    ):
        # type: (...) -> Optional[Item]
        line = f.readline().rstrip()
        if not line:
            return None
        headers = json.loads(line)
        length = headers["length"]
        payload = f.read(length)
        if headers.get("type") == "event":
            rv = cls(headers=headers, payload=PayloadRef(json=json.loads(payload)))
        else:
            rv = cls(headers=headers, payload=payload)
        f.readline()
        return rv

    @classmethod
    def deserialize(
        cls, bytes  # type: bytes
    ):
        # type: (...) -> Optional[Item]
        return cls.deserialize_from(io.BytesIO(bytes))

    def __repr__(self):
        # type: (...) -> str
        return "<Item headers=%r payload=%r>" % (
            self.headers,
            self.payload,
        )

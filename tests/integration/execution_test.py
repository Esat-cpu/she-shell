import subprocess
import pytest
import shutil


@pytest.fixture
def run_with_she(she_path):
    def run(command: str, *, cwd=None):
        return subprocess.run(
            [she_path, "-c", command],
            capture_output=True, text=True, cwd=cwd
        )
    return run


@pytest.fixture
def run_file_with_she(she_path):
    def run(filename: str, *, cwd=None):
        return subprocess.run(
            [she_path, filename],
            capture_output=True, text=True, cwd=cwd
        )
    return run


def read_from_file(filename: str) -> str:
    with open(filename, 'r') as f:
        return f.read()


def test_logical_operators(run_with_she):
    command = "echo test || echo or && echo and"
    res = run_with_she(command)

    assert res.stdout == "test\nand\n"
    assert res.stderr == ""


def test_redirection_append_and_truncate(run_with_she, tmp_path):
    command = "echo hello > test.txt"
    res = run_with_she(command, cwd=tmp_path)

    text = read_from_file(tmp_path / "test.txt")

    assert res.stdout == ""
    assert text == "hello\n"

    command = "echo world >> test.txt"
    res = run_with_she(command, cwd=tmp_path)

    text = read_from_file(tmp_path / "test.txt")

    assert res.stdout == ""
    assert text == "hello\nworld\n"

    command = "echo only this > test.txt"
    res = run_with_she(command, cwd=tmp_path)

    text = read_from_file(tmp_path / "test.txt")

    assert text == "only this\n"


def test_redirection_stderr(run_with_she, tmp_path):
    command = "exit 1 2 2> test.txt"
    res = run_with_she(command, cwd=tmp_path)

    text = read_from_file(tmp_path / "test.txt")

    assert res.stderr == ""
    assert text != ""
    assert res.returncode == 1


def test_multiple_redirections(she_path, run_with_she, tmp_path):
    command = f"{she_path} -c 'exit 1 2; echo hey' > out.txt 2> err.txt"
    res = run_with_she(command, cwd=tmp_path)

    out = read_from_file(tmp_path / "out.txt")
    err = read_from_file(tmp_path / "err.txt")

    assert res.stdout == ""
    assert res.stderr == ""
    assert out == "hey\n"
    assert err != ""


def test_sequence(run_with_she):
    command = "echo helo; echo other;"
    res = run_with_she(command)

    assert res.stdout == "helo\nother\n"

    command = "; echo helo"
    res = run_with_she(command)

    assert res.stdout == ""
    assert res.stderr.endswith("Parse error near ';'\n")


def test_pipe(run_with_she):
    if shutil.which("tr") is None:
        pytest.skip("tr is not available")

    command = "echo This is a tEst | tr 't' 'T' | tr 's' 'S' | tr 'E' 'e'"
    res = run_with_she(command)

    assert res.stdout == "ThiS iS a TeST\n"
    assert res.stderr == ""


def test_script_execution_comments_and_exit(run_file_with_she, tmp_path):
    with open(tmp_path / "test.sh", 'w') as f:
        f.write("""
        # this is test
        # echo yes it is
        echo hello
        exit 1
        echo world
        """)

    res = run_file_with_she(tmp_path / "test.sh")

    assert res.stdout == "hello\n"
    assert res.stderr == ""


def test_file_error(run_file_with_she, tmp_path):
    with open(tmp_path / "test.sh", 'w') as f:
        f.write("""
        echo hey > non_existing/weird_thing
        echo wow
        exit 5
        """)

    res = run_file_with_she(tmp_path / "test.sh")

    assert res.returncode == 5
    assert res.stdout == "wow\n"


def test_file_error_sets_exit_code(run_with_she, tmp_path):
    command = "echo hey > non_existing/weird_thing"
    res = run_with_she(command, cwd=tmp_path)

    assert res.returncode != 0


def test_input_from_pipe(she_path, run_with_she):
    command = f"echo 'echo helo\necho world' | {she_path}"
    res = run_with_she(command)

    assert res.stdout == "helo\nworld\n"
    assert res.stderr == ""

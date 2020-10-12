import moo
import traceback

def _tassert(cond, msg):
    st = traceback.extract_stack()
    loc = st[len(st) - 3]
    if not msg:
        msg = "`%s'" % loc[3]
    if loc[2] != '<module>':
        msg = 'in function %s: %s' % (loc[2], msg)
    moo.test_assert_impl(bool(cond), msg, loc[0], loc[1])

def tassert(cond, msg=None):
    _tassert(cond, msg)

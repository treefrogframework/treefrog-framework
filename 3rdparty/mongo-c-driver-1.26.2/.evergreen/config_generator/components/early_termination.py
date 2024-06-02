from config_generator.etc.function import Function
from config_generator.etc.utils import bash_exec


class EarlyTermination(Function):
    name = 'early-termination'
    commands = [
        bash_exec(
            script='''\
                echo 'EVERGREEN HOST WAS UNEXPECTEDLY TERMINATED!!!' 1>&2
                echo 'Restart this Evergreen task and try again!' 1>&2
            '''
        ),
    ]

    @classmethod
    def call(cls, **kwargs):
        return cls.default_call(**kwargs)


def functions():
    return EarlyTermination.defn()

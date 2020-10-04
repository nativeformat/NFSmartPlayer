import sys
import json
import datetime
import jsondiff


# Helper class for validating NFLogging integration tests
class ValidateLogging:
    def validate(self, integration_test, nf_logging_file):
        # Integration tests send nf log message to nf_logging file.
        # This function validates that log.
        expected_file = integration_test['graph'].replace(
            "resources/",
            "resources/nf-logging/expected/")
        expected = json.loads(open(expected_file, 'r').read())

        # Expected fixture is formatted as one line of json per message
        # This matches the output of JSON mode NF logging.
        actual = []
        with open(nf_logging_file, 'r') as f:
            lines = f.readlines()
            for i in range(len(lines)):
                line = lines[i]
                try:
                    actual.append(json.loads(line))
                except ValueError:
                    self.build_print("Failed to decode: " + line)
                    exit(1)

        self.__validateMessageStructure(expected, actual)
        self.__validateSumField(expected, actual, 'renderDuration')
        self.__validateSumField(expected, actual, 'sourceDuration')

    def build_print(self, print_string):
        print print_string
        sys.stdout.flush()

    def __removeStartTime(self, partialEndSample, field):
        # Validate start time format, then remove it since its variable.
        try:
            datetime.datetime.strptime(
                partialEndSample[field],
                '%Y-%m-%dT%H:%M:%SZ')
        except ValueError:
            self.build_print("Invalid startTime: ",
                             partialEndSample[field])
            sys.exit(1)
        partialEndSample.pop(field)

    def __removeField(self, hash, field):
        # Remove a generic field from a hash after validating it is present
        if field not in hash:
            self.build_print("Missing " + field)
            sys.exit(1)
        hash.pop(field)

    def __parseTime(self, str):
        return float(str.replace("s", ""))

    def __normalizeMessages(self, messages):
        # Remove or round nondeterministic fields
        copy = json.loads(json.dumps(messages))
        for message in copy:
            self.__removeField(
                message['clientInfo'],
                "playerBuildId")
            for submessage in message['message']:
                partialEndSample = submessage['partialEndSample']
                # validate and remove fields that are variable
                self.__removeStartTime(partialEndSample, 'startTime')
                self.__removeField(partialEndSample, 'scorePlaybackId')
                # Round durations to precision .1 second
                partialEndSample['renderDuration'] = round(
                    self.__parseTime(partialEndSample['renderDuration']), 1)
                partialEndSample['sourceDuration'] = round(
                    self.__parseTime(partialEndSample['sourceDuration']), 1)
                partialEndSample['graphOffset'] = round(
                    self.__parseTime(partialEndSample['graphOffset']), 1)
                partialEndSample['sourceOffset'] = round(
                    self.__parseTime(partialEndSample['sourceOffset']), 1)
        return json.dumps(copy)

    def __sumMessageFields(self, messages, field):
        # For each node, return the sum of this field with ms level accuracy
        sums = {}
        for message in messages:
            for submessage in message['message']:
                pes = submessage['partialEndSample']
                if pes['nodeId'] not in sums:
                    sums[pes['nodeId']] = 0.0
                sums[pes['nodeId']] += self.__parseTime(pes[field])
        return {k: round(v, 3) for k, v in sums.items()}

    def __validateMessageStructure(self, expected, actual):
        # Normalize messages to eliminate or round off nondeterministic
        # fields, then diff. Fail verbosely if match fails.
        expected_str = self.__normalizeMessages(expected)
        actual_str = self.__normalizeMessages(actual)
        if expected_str != actual_str:
            self.build_print("Actual:")
            self.build_print(actual_str)
            self.build_print("Expected:")
            self.build_print(expected_str)
            self.build_print("Actual and expected differ.")
            self.build_print("Diff:")
            self.build_print(jsondiff.diff(expected, actual))
            sys.exit(1)

    def __validateSumField(self, expected, actual, field):
        # For the input field, sum all values per node,
        # then compare each value for discrepancies
        expectedSums = json.dumps(
            self.__sumMessageFields(expected, field),
            sort_keys=True)
        actualSums = json.dumps(
            self.__sumMessageFields(actual, field),
            sort_keys=True)

        if expectedSums != actualSums:
            self.build_print("Actual Sum:")
            self.build_print(expectedSums)
            self.build_print("Expected Sum:")
            self.build_print(actualSums)
            self.build_print("Actual and expected differ for field %s" % field)
            self.build_print("Diff:")
            self.build_print(jsondiff.diff(
                expectedSums,
                actualSums))
            sys.exit(1)

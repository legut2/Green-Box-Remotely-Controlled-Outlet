#include <NotecardPseudoSensor.h>
#include <Notecard.h>
#include <optional>

// Helper Functions
J* create_request(const char* file, const char* tracker) {
    J *req = NoteNewRequest("note.changes");
    JAddStringToObject(req, "file", file);
    JAddStringToObject(req, "tracker", tracker);
    JAddBoolToObject(req, "start", true);
    JAddBoolToObject(req, "delete", true);
    JAddNumberToObject(req, "max", 300);
    return req;
}

void log_error(const char* message, Notecard &notecard) {
    notecard.logDebug(message);
    Serial.println(message);
}

// Utility function to get a string value from a JSON object
const char* getJsonStringValue(J* jsonObject, const char* key) {
    J* item = JGetObjectItem(jsonObject, key);
    return item != nullptr ? item->valuestring : nullptr;
}

// Utility function to get an integer value from a JSON object
long getJsonIntValue(J* jsonObject, const char* key) {
    J* item = JGetObjectItem(jsonObject, key);
    return item != nullptr ? item->valueint : -1;
}

std::optional<bool> getJsonBoolValue(J* jsonObject, const char* key) {
    J* item = JGetObjectItem(jsonObject, key);
    if (item != nullptr) {
        if (item->type == JTrue) {
            return true;
        } else if (item->type == JFalse) {
            return false;
        }
    }
    return std::nullopt; // Return empty std::optional for other cases
}

// Main function to process a single note item
std::optional<bool> process_note_item(J* noteKeyItem) {
    std::optional<bool> desired_relay_state;
    if (noteKeyItem == nullptr) {
        Serial.println("noteKeyItem is NULL");
        return std::optional<bool>();  // Return dynamically allocated empty string
    }

    Serial.println("Looping through note keys");
    Serial.print("Key: ");
    Serial.println(noteKeyItem->string);
    JPrint(noteKeyItem);

    J* bodyItem = JGetObject(noteKeyItem, "body");
    if (bodyItem == nullptr) {
        Serial.println("bodyItem is NULL");
        return std::optional<bool>();
    }

    JPrint(bodyItem);

    desired_relay_state = getJsonBoolValue(bodyItem, "desired_relay_state");
    Serial.println(desired_relay_state.value() ? "true" : "false");

    long timeValue = getJsonIntValue(noteKeyItem, "time");

    if (timeValue != -1) {
        return desired_relay_state;
    } else {
        Serial.println("timeObject is NULL");
        return std::optional<bool>();  // Return dynamically allocated empty string
    }
}

std::optional<bool> process_notes(J* notes, Notecard &notecard) {
    std::optional<bool> return_value;
    if (notes == nullptr) {
        log_error("Notes are NULL", notecard);
        return std::optional<bool>();  // Return dynamically allocated empty string
    }

    J* noteKeyItem;
    JArrayForEach(noteKeyItem, notes) {
        return_value = process_note_item(noteKeyItem);
    }
    return return_value; // Caller must free this
}

// Main Functions Blues.io
J* get_notes(J* req, Notecard &notecard) {
    J* notes_obj = nullptr;
    J* rsp = notecard.requestAndResponse(req);
    if (notecard.responseError(rsp)) {
        log_error("No notes available", notecard);
        return nullptr;
    }

    J* temp_notes_obj = JGetObject(rsp, "notes");
    if (!temp_notes_obj) {
        log_error("Notes object not found", notecard);
        notecard.deleteResponse(rsp);
        return nullptr;
    }

    char* notes_str = JPrintUnformatted(temp_notes_obj);
    notes_obj = JParse(notes_str);
    JFree(notes_str);
    notecard.deleteResponse(rsp);
    return notes_obj;
}

// checks changes in notes and loops through the changes and returns value that has been appearing recently
std::optional<bool> check_notes_for_digital_relay_control_command(Notecard &notecard) {
    std::optional<bool> ret_value;
    J* req = create_request("data.qi", "inbound-tracker");
    J* notes = get_notes(req, notecard);
    ret_value = process_notes(notes, notecard);
    JDelete(notes);
    return ret_value;
}
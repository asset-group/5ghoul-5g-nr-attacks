#!/usr/bin/env python3

import os
import sys
import base64
import argparse
from email.message import EmailMessage
from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.errors import HttpError

TAG = '[AlertSender:Gmail]'


def get_credentials(cred_path=None):

    SCOPES = [
        "https://www.googleapis.com/auth/gmail.send"
    ]

    script_path = os.path.dirname(os.path.realpath(__file__))
    token_path = os.path.join(script_path, 'token.json')
    creds = None
    print_token_status = False

    if cred_path is None:
        cred_path = os.path.join(script_path, 'credentials.json')
    else:
        print_token_status = True

    if os.path.exists(token_path):
        creds = Credentials.from_authorized_user_file(token_path, SCOPES)

    # If there are no (valid) credentials available, let the user log in.
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            print(F'{TAG} Refreshing token.json')
            creds.refresh(Request())
        else:
            print(F'{TAG} Creating token.json')
            flow = InstalledAppFlow.from_client_secrets_file(cred_path, SCOPES)
            creds = flow.run_local_server(port=0)
        # Save the credentials for the next run
        with open(token_path, 'w') as token:
            token.write(creds.to_json())
            print(F'{TAG} Token created successfully')

    if not creds.valid:
        print(F'{TAG} Error: Token is invalid')
        exit(0)
    elif creds.valid and print_token_status:
        print(F'{TAG} Token is valid')

    return creds


def send_email(email_to, msg_title, msg_body):
    """Create and send an email message
    Print the returned  message id
    Returns: Message object, including message id
    """

    # get credentials
    creds = get_credentials()

    print(F'{TAG} Sending email...')
    try:
        service = build('gmail', 'v1', credentials=creds)
        message = EmailMessage()

        message['To'] = email_to
        message['Subject'] = msg_title

        message.set_content(msg_body)

        # encoded message
        encoded_message = base64.urlsafe_b64encode(message.as_bytes()) \
            .decode()

        create_message = {
            'raw': encoded_message
        }

        send_message = (service.users().messages().send(
            userId="me", body=create_message).execute())

        message_id = send_message["id"]
        message_name = message['Subject']

        print(F'{TAG} Email Sent! Title: "{message_name}", Message Id: {message_id}')

    except HttpError as error:
        print(F'{TAG} An error occurred: {error}')
        send_message = None

    return send_message


if __name__ == '__main__':
    # get arguments
    parser = argparse.ArgumentParser(description='Send email via Google Gmail API. Requires credentials.json')
    group = parser.add_argument_group('Credentials')
    group.add_argument('-c','--credential', help='Verify credentials.json and generate token.json')

    subparser = parser.add_subparsers()
    parser_send = subparser.add_parser('send', help='Send email')
    group = parser_send.add_argument_group('Send Email')
    group.add_argument('-t', '--to', help='Email to', required=True)
    group.add_argument('-s', '--subject',
                        help='Email subject', required=True)
    group.add_argument('-b', '--body', help='Email body', required=True)
    
    args = parser.parse_args()

    if len(sys.argv) < 2:
        parser.print_help()
        exit(1)

    if args.credential:
        get_credentials(os.path.abspath(args.credential))
        exit(0)

    send_email(args.to, args.subject, args.body)
    exit(0)

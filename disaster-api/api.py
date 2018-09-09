import flask
from flask import Flask, render_template, request, make_response, current_app
import re
from sklearn.externals import joblib
from nltk.corpus import stopwords
import pandas as pd
from datetime import timedelta
from functools import update_wrapper

app = Flask(__name__)
@app.route("/")
@app.route("/index")
def index():
   return flask.render_template('index.html')

def preprocessTweet(tweet): # preprocess tweets
    tweet = re.sub(r'\d+', '', str(tweet)) # remove numbers
    tweet = tweet.lower() # convert to lower case
    tweet = re.sub('((www\.[^\s]+)|(https?://[^\s]+))', 'URL', tweet) # convert www.* or https?://* to URL
    tweet = re.sub('@[^\s]+','AT_USER',tweet) # convert @username to AT_USER
    tweet = re.sub('[\s]+', ' ', tweet) # remove additional white spaces
    tweet = re.sub(r'#([^\s]+)', r'\1', tweet) # replace #word (hashtags) with word
    tweet = tweet.strip('\'"') # trim
    return tweet

def replaceTwoOrMore(s):
    #look for 2 or more repetitions of character and replace with the character itself
    pattern = re.compile(r"(.)\1{1,}", re.DOTALL)
    return pattern.sub(r"\1\1", s)

def getStopWordList(stopWordListFileName): # get stopword list
    sw = [] # create list of stopwords

    nltk_stopwords = stopwords.words('english')
    for w in nltk_stopwords:
        sw.append(w)
    sw.append('AT_USER') # special stopwords from preprocessTweet function
    sw.append('URL')

    fp = open(stopWordListFileName, 'r') # load in any more custom stopwords from file
    line = fp.readline()
    while line:
        word = line.strip()
        sw.append(word)
        line = fp.readline()
    fp.close()
    return sw

def getFeatureVector(tweet):
    stopword_list = getStopWordList('data/stopwords.txt')
    featureVector = []
    #split tweet into words
    words = tweet.split()
    for w in words:
        w = replaceTwoOrMore(w) # replace two or more with two occurrences
        w = w.strip('\'"?,.') # strip punctuation
        val = re.search(r"^[a-zA-Z][a-zA-Z0-9]*$", w) # check if the word starts with an alphabet
        if(w in stopword_list or val is None): # ignore if it is a stop word
            continue
        else:
            featureVector.append(w.lower())
    return featureVector


def extract_features(tweet): # Determine if tweet contains a feature word

    tweet_words = set(tweet)
    features = {}
    for word in disaster_featurelist:
        features['contains(%s)' % word] = (word in tweet_words)
    return features

@app.route('/predict', methods=['POST'])
def make_prediction():
    if request.method=='POST':
        message = request.form['message']
        if not message:
            return render_template('index.html', label="No message")
        prediction = model.classify(extract_features(getFeatureVector(preprocessTweet(message))))
        return render_template('index.html', label=prediction)

def crossdomain(origin=None, methods=None, headers=None, max_age=21600,
                attach_to_all=True, automatic_options=True):
    """Decorator function that allows crossdomain requests.
      Courtesy of
      https://blog.skyred.fi/articles/better-crossdomain-snippet-for-flask.html
    """
    if methods is not None:
        methods = ', '.join(sorted(x.upper() for x in methods))
    if headers is not None and not isinstance(headers, list):
        headers = ', '.join(x.upper() for x in headers)
    if not isinstance(origin, list):
        origin = ', '.join(origin)
    if isinstance(max_age, timedelta):
        max_age = max_age.total_seconds()

    def get_methods():
        """ Determines which methods are allowed
        """
        if methods is not None:
            return methods

        options_resp = current_app.make_default_options_response()
        return options_resp.headers['allow']

    def decorator(f):
        """The decorator function
        """
        def wrapped_function(*args, **kwargs):
            """Caries out the actual cross domain code
            """
            if automatic_options and request.method == 'OPTIONS':
                resp = current_app.make_default_options_response()
            else:
                resp = make_response(f(*args, **kwargs))
            if not attach_to_all and request.method != 'OPTIONS':
                return resp

            h = resp.headers
            h['Access-Control-Allow-Origin'] = origin
            h['Access-Control-Allow-Methods'] = get_methods()
            h['Access-Control-Max-Age'] = str(max_age)
            h['Access-Control-Allow-Credentials'] = 'true'
            h['Access-Control-Allow-Headers'] = \
                "Origin, X-Requested-With, Content-Type, Accept, Authorization"
            if headers is not None:
                h['Access-Control-Allow-Headers'] = headers
            return resp

        f.provide_automatic_options = False
        return update_wrapper(wrapped_function, f)
    return decorator

@app.route('/predict/<message>', methods=['GET', 'POST', 'OPTIONS'])
@crossdomain(origin='*')
def make_prediction_request(message=None):
    if message:
        return model.classify(extract_features(getFeatureVector(preprocessTweet(message))))
    else:
        return ''

if __name__ == '__main__':
    model = joblib.load('data/disaster_NBClassifier.pkl')
    good_files = ['data/2012_Colorado_wildfires-tweets_labeled.csv', 'data/2012_Philipinnes_floods-tweets_labeled.csv',
                  'data/2013_Alberta_floods-tweets_labeled.csv', 'data/2013_Australia_bushfire-tweets_labeled.csv',
                  'data/2013_Boston_bombings-tweets_labeled.csv', 'data/2013_Colorado_floods-tweets_labeled.csv',
                  'data/2013_Glasgow_helicopter_crash-tweets_labeled.csv',
                  'data/2013_LA_airport_shootings-tweets_labeled.csv', 'data/2013_NY_train_crash-tweets_labeled.csv',
                  'data/2013_Queensland_floods-tweets_labeled.csv', 'data/2013_West_Texas_explosion-tweets_labeled.csv']

    disaster_tweets = []
    disaster_featurelist = []

    for file in good_files:

        disaster_train = pd.read_csv(file, encoding="ISO-8859-1") #latin1 encoding

        for i in range(len(disaster_train)):
            sentiment = ''
            if disaster_train[' Informativeness'][i] == 'Related and informative':
                sentiment = 'Informative'
            else:
                sentiment = 'Not Informative'
            tweet = disaster_train[' Tweet Text'][i]
            preprocessedTweet = preprocessTweet(tweet)
            featureVector = getFeatureVector(preprocessedTweet)
            disaster_featurelist.extend(featureVector)
            disaster_tweets.append((featureVector, sentiment))
    app.run(host='0.0.0.0', port=8000, debug=True)
